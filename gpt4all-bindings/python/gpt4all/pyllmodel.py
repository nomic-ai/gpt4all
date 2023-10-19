import atexit
import ctypes
import importlib.resources
import logging
import os
import platform
import re
import subprocess
import sys
import threading
from contextlib import ExitStack
from queue import Queue
from typing import Callable, Iterable, List

logger: logging.Logger = logging.getLogger(__name__)


file_manager = ExitStack()
atexit.register(file_manager.close)  # clean up files on exit

# TODO: provide a config file to make this more robust
MODEL_LIB_PATH = file_manager.enter_context(importlib.resources.as_file(
    importlib.resources.files("gpt4all") / "llmodel_DO_NOT_MODIFY" / "build",
))


def load_llmodel_library():
    ext = {"Darwin": "dylib", "Linux": "so", "Windows": "dll"}[platform.system()]

    try:
        # Linux, Windows, MinGW
        lib = ctypes.CDLL(str(MODEL_LIB_PATH / f"libllmodel.{ext}"))
    except FileNotFoundError:
        if ext != 'dll':
            raise
        # MSVC
        lib = ctypes.CDLL(str(MODEL_LIB_PATH / "llmodel.dll"))

    return lib


llmodel = load_llmodel_library()


class LLModelError(ctypes.Structure):
    _fields_ = [("message", ctypes.c_char_p), ("code", ctypes.c_int32)]


class LLModelPromptContext(ctypes.Structure):
    _fields_ = [
        ("logits", ctypes.POINTER(ctypes.c_float)),
        ("logits_size", ctypes.c_size_t),
        ("tokens", ctypes.POINTER(ctypes.c_int32)),
        ("tokens_size", ctypes.c_size_t),
        ("n_past", ctypes.c_int32),
        ("n_ctx", ctypes.c_int32),
        ("n_predict", ctypes.c_int32),
        ("top_k", ctypes.c_int32),
        ("top_p", ctypes.c_float),
        ("temp", ctypes.c_float),
        ("n_batch", ctypes.c_int32),
        ("repeat_penalty", ctypes.c_float),
        ("repeat_last_n", ctypes.c_int32),
        ("context_erase", ctypes.c_float),
    ]

class LLModelGPUDevice(ctypes.Structure):
    _fields_ = [
        ("index", ctypes.c_int32),
        ("type", ctypes.c_int32),
        ("heapSize", ctypes.c_size_t),
        ("name", ctypes.c_char_p),
        ("vendor", ctypes.c_char_p),
    ]

# Define C function signatures using ctypes
llmodel.llmodel_model_create.argtypes = [ctypes.c_char_p]
llmodel.llmodel_model_create.restype = ctypes.c_void_p

llmodel.llmodel_model_create2.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(LLModelError)]
llmodel.llmodel_model_create2.restype = ctypes.c_void_p

llmodel.llmodel_model_destroy.argtypes = [ctypes.c_void_p]
llmodel.llmodel_model_destroy.restype = None

llmodel.llmodel_loadModel.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
llmodel.llmodel_loadModel.restype = ctypes.c_bool
llmodel.llmodel_required_mem.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
llmodel.llmodel_required_mem.restype = ctypes.c_size_t
llmodel.llmodel_isModelLoaded.argtypes = [ctypes.c_void_p]
llmodel.llmodel_isModelLoaded.restype = ctypes.c_bool

PromptCallback = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_int32)
ResponseCallback = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_int32, ctypes.c_char_p)
RecalculateCallback = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_bool)

llmodel.llmodel_prompt.argtypes = [
    ctypes.c_void_p,
    ctypes.c_char_p,
    PromptCallback,
    ResponseCallback,
    RecalculateCallback,
    ctypes.POINTER(LLModelPromptContext),
]

llmodel.llmodel_prompt.restype = None

llmodel.llmodel_embedding.argtypes = [
    ctypes.c_void_p,
    ctypes.c_char_p,
    ctypes.POINTER(ctypes.c_size_t),
]

llmodel.llmodel_embedding.restype = ctypes.POINTER(ctypes.c_float)

llmodel.llmodel_free_embedding.argtypes = [ctypes.POINTER(ctypes.c_float)]
llmodel.llmodel_free_embedding.restype = None

llmodel.llmodel_setThreadCount.argtypes = [ctypes.c_void_p, ctypes.c_int32]
llmodel.llmodel_setThreadCount.restype = None

llmodel.llmodel_set_implementation_search_path.argtypes = [ctypes.c_char_p]
llmodel.llmodel_set_implementation_search_path.restype = None

llmodel.llmodel_threadCount.argtypes = [ctypes.c_void_p]
llmodel.llmodel_threadCount.restype = ctypes.c_int32

llmodel.llmodel_set_implementation_search_path(str(MODEL_LIB_PATH).replace("\\", r"\\").encode("utf-8"))

llmodel.llmodel_available_gpu_devices.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_int32)]
llmodel.llmodel_available_gpu_devices.restype = ctypes.POINTER(LLModelGPUDevice)

llmodel.llmodel_gpu_init_gpu_device_by_string.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_char_p]
llmodel.llmodel_gpu_init_gpu_device_by_string.restype = ctypes.c_bool

llmodel.llmodel_gpu_init_gpu_device_by_struct.argtypes = [ctypes.c_void_p, ctypes.POINTER(LLModelGPUDevice)]
llmodel.llmodel_gpu_init_gpu_device_by_struct.restype = ctypes.c_bool

llmodel.llmodel_gpu_init_gpu_device_by_int.argtypes = [ctypes.c_void_p, ctypes.c_int32]
llmodel.llmodel_gpu_init_gpu_device_by_int.restype = ctypes.c_bool

llmodel.llmodel_has_gpu_device.argtypes = [ctypes.c_void_p]
llmodel.llmodel_has_gpu_device.restype = ctypes.c_bool

ResponseCallbackType = Callable[[int, str], bool]
RawResponseCallbackType = Callable[[int, bytes], bool]


def empty_response_callback(token_id: int, response: str) -> bool:
    return True


class LLModel:
    """
    Base class and universal wrapper for GPT4All language models
    built around llmodel C-API.

    Attributes
    ----------
    model: llmodel_model
        Ctype pointer to underlying model
    model_name: str
        Model name
    """

    def __init__(self):
        self.model = None
        self.model_name = None
        self.context = None
        self.llmodel_lib = llmodel

        self.buffer = bytearray()
        self.buff_expecting_cont_bytes: int = 0

    def __del__(self):
        if self.model is not None:
            self.llmodel_lib.llmodel_model_destroy(self.model)

    def memory_needed(self, model_path: str) -> int:
        model_path_enc = model_path.encode("utf-8")
        self.model = llmodel.llmodel_model_create(model_path_enc)

        if self.model is not None:
            return llmodel.llmodel_required_mem(self.model, model_path_enc)
        else:
            raise ValueError("Unable to instantiate model")

    def list_gpu(self, model_path: str) -> list:
        """
        Lists available GPU devices that satisfy the model's memory requirements.

        Parameters
        ----------
        model_path : str
            Path to the model.

        Returns
        -------
        list
            A list of LLModelGPUDevice structures representing available GPU devices.
        """
        if self.model is not None:
            model_path_enc = model_path.encode("utf-8")
            mem_required = llmodel.llmodel_required_mem(self.model, model_path_enc)
        else:
            mem_required = self.memory_needed(model_path)
        num_devices = ctypes.c_int32(0)
        devices_ptr = self.llmodel_lib.llmodel_available_gpu_devices(self.model, mem_required, ctypes.byref(num_devices))
        if not devices_ptr:
            raise ValueError("Unable to retrieve available GPU devices")
        devices = [devices_ptr[i] for i in range(num_devices.value)]
        return devices

    def init_gpu(self, model_path: str, device: str):
        if self.model is not None:
            model_path_enc = model_path.encode("utf-8")
            mem_required = llmodel.llmodel_required_mem(self.model, model_path_enc)
        else:
            mem_required = self.memory_needed(model_path)
        device_enc = device.encode("utf-8")
        success = self.llmodel_lib.llmodel_gpu_init_gpu_device_by_string(self.model, mem_required, device_enc)
        if not success:
            # Retrieve all GPUs without considering memory requirements.
            num_devices = ctypes.c_int32(0)
            all_devices_ptr = self.llmodel_lib.llmodel_available_gpu_devices(self.model, 0, ctypes.byref(num_devices))
            if not all_devices_ptr:
                raise ValueError("Unable to retrieve list of all GPU devices")
            all_gpus = [all_devices_ptr[i].name.decode('utf-8') for i in range(num_devices.value)]

            # Retrieve GPUs that meet the memory requirements using list_gpu
            available_gpus = [device.name.decode('utf-8') for device in self.list_gpu(model_path)]

            # Identify GPUs that are unavailable due to insufficient memory or features
            unavailable_gpus = set(all_gpus) - set(available_gpus)

            # Formulate the error message
            error_msg = "Unable to initialize model on GPU: '{}'.".format(device)
            error_msg += "\nAvailable GPUs: {}.".format(available_gpus)
            error_msg += "\nUnavailable GPUs due to insufficient memory or features: {}.".format(unavailable_gpus)
            raise ValueError(error_msg)

    def load_model(self, model_path: str) -> bool:
        """
        Load model from a file.

        Parameters
        ----------
        model_path : str
            Model filepath

        Returns
        -------
        True if model loaded successfully, False otherwise
        """
        model_path_enc = model_path.encode("utf-8")
        err = LLModelError()
        self.model = llmodel.llmodel_model_create2(model_path_enc, b"auto", ctypes.byref(err))

        if self.model is None:
            raise ValueError(f"Unable to instantiate model: code={err.code}, {err.message.decode()}")

        llmodel.llmodel_loadModel(self.model, model_path_enc)

        filename = os.path.basename(model_path)
        self.model_name = os.path.splitext(filename)[0]

        if llmodel.llmodel_isModelLoaded(self.model):
            return True
        else:
            return False

    def set_thread_count(self, n_threads):
        if not llmodel.llmodel_isModelLoaded(self.model):
            raise Exception("Model not loaded")
        llmodel.llmodel_setThreadCount(self.model, n_threads)

    def thread_count(self):
        if not llmodel.llmodel_isModelLoaded(self.model):
            raise Exception("Model not loaded")
        return llmodel.llmodel_threadCount(self.model)

    def _set_context(
        self,
        n_predict: int = 4096,
        top_k: int = 40,
        top_p: float = 0.9,
        temp: float = 0.1,
        n_batch: int = 8,
        repeat_penalty: float = 1.2,
        repeat_last_n: int = 10,
        context_erase: float = 0.75,
        reset_context: bool = False,
    ):
        if self.context is None:
            self.context = LLModelPromptContext(
                logits_size=0,
                tokens_size=0,
                n_past=0,
                n_ctx=0,
                n_predict=n_predict,
                top_k=top_k,
                top_p=top_p,
                temp=temp,
                n_batch=n_batch,
                repeat_penalty=repeat_penalty,
                repeat_last_n=repeat_last_n,
                context_erase=context_erase,
            )
        elif reset_context:
            self.context.n_past = 0

        self.context.n_predict = n_predict
        self.context.top_k = top_k
        self.context.top_p = top_p
        self.context.temp = temp
        self.context.n_batch = n_batch
        self.context.repeat_penalty = repeat_penalty
        self.context.repeat_last_n = repeat_last_n
        self.context.context_erase = context_erase

    def generate_embedding(self, text: str) -> List[float]:
        if not text:
            raise ValueError("Text must not be None or empty")

        embedding_size = ctypes.c_size_t()
        c_text = ctypes.c_char_p(text.encode('utf-8'))
        embedding_ptr = llmodel.llmodel_embedding(self.model, c_text, ctypes.byref(embedding_size))
        embedding_array = [embedding_ptr[i] for i in range(embedding_size.value)]
        llmodel.llmodel_free_embedding(embedding_ptr)
        return list(embedding_array)

    def prompt_model(
        self,
        prompt: str,
        callback: ResponseCallbackType,
        n_predict: int = 4096,
        top_k: int = 40,
        top_p: float = 0.9,
        temp: float = 0.1,
        n_batch: int = 8,
        repeat_penalty: float = 1.2,
        repeat_last_n: int = 10,
        context_erase: float = 0.75,
        reset_context: bool = False,
    ):
        """
        Generate response from model from a prompt.

        Parameters
        ----------
        prompt: str
            Question, task, or conversation for model to respond to
        callback(token_id:int, response:str): bool
            The model sends response tokens to callback

        Returns
        -------
        None
        """

        self.buffer.clear()
        self.buff_expecting_cont_bytes = 0

        logger.info(
            "LLModel.prompt_model -- prompt:\n"
            + "%s\n"
            + "===/LLModel.prompt_model -- prompt/===",
            prompt,
        )

        prompt_bytes = prompt.encode("utf-8")
        prompt_ptr = ctypes.c_char_p(prompt_bytes)

        self._set_context(
            n_predict=n_predict,
            top_k=top_k,
            top_p=top_p,
            temp=temp,
            n_batch=n_batch,
            repeat_penalty=repeat_penalty,
            repeat_last_n=repeat_last_n,
            context_erase=context_erase,
            reset_context=reset_context,
        )

        llmodel.llmodel_prompt(
            self.model,
            prompt_ptr,
            PromptCallback(self._prompt_callback),
            ResponseCallback(self._callback_decoder(callback)),
            RecalculateCallback(self._recalculate_callback),
            self.context,
        )


    def prompt_model_streaming(
        self, prompt: str, callback: ResponseCallbackType = empty_response_callback, **kwargs
    ) -> Iterable[str]:
        # Symbol to terminate from generator
        TERMINATING_SYMBOL = object()

        output_queue: Queue = Queue()

        # Put response tokens into an output queue
        def _generator_callback_wrapper(callback: ResponseCallbackType) -> ResponseCallbackType:
            def _generator_callback(token_id: int, response: str):
                nonlocal callback

                if callback(token_id, response):
                    output_queue.put(response)
                    return True

                return False

            return _generator_callback

        def run_llmodel_prompt(prompt: str, callback: ResponseCallbackType, **kwargs):
            self.prompt_model(prompt, callback, **kwargs)
            output_queue.put(TERMINATING_SYMBOL)

        # Kick off llmodel_prompt in separate thread so we can return generator
        # immediately
        thread = threading.Thread(
            target=run_llmodel_prompt,
            args=(prompt, _generator_callback_wrapper(callback)),
            kwargs=kwargs,
        )
        thread.start()

        # Generator
        while True:
            response = output_queue.get()
            if response is TERMINATING_SYMBOL:
                break
            yield response

    def _callback_decoder(self, callback: ResponseCallbackType) -> RawResponseCallbackType:
        def _raw_callback(token_id: int, response: bytes) -> bool:
            nonlocal self, callback

            decoded = []

            for byte in response:
                
                bits = "{:08b}".format(byte)
                (high_ones, _, _) = bits.partition('0')

                if len(high_ones) == 1: 
                    # continuation byte
                    self.buffer.append(byte)
                    self.buff_expecting_cont_bytes -= 1

                else: 
                    # beginning of a byte sequence
                    if len(self.buffer) > 0:
                        decoded.append(self.buffer.decode('utf-8', 'replace'))

                        self.buffer.clear()

                    self.buffer.append(byte)
                    self.buff_expecting_cont_bytes = max(0, len(high_ones) - 1)

                if self.buff_expecting_cont_bytes <= 0: 
                    # received the whole sequence or an out of place continuation byte
                    decoded.append(self.buffer.decode('utf-8', 'replace'))

                    self.buffer.clear()
                    self.buff_expecting_cont_bytes = 0
                    
            if len(decoded) == 0 and self.buff_expecting_cont_bytes > 0:
                # wait for more continuation bytes
                return True
            
            return callback(token_id, ''.join(decoded))     

        return _raw_callback

    # Empty prompt callback
    @staticmethod
    def _prompt_callback(token_id: int) -> bool:
        return True

    # Empty recalculate callback
    @staticmethod
    def _recalculate_callback(is_recalculating: bool) -> bool:
        return is_recalculating
