import ctypes
import logging
import os
import platform
from queue import Queue
import re
import subprocess
import sys
import threading
from typing import Callable, Iterable, List

import pkg_resources

logger: logging.Logger = logging.getLogger(__name__)


# TODO: provide a config file to make this more robust
LLMODEL_PATH = os.path.join("llmodel_DO_NOT_MODIFY", "build").replace("\\", "\\\\")
MODEL_LIB_PATH = str(pkg_resources.resource_filename("gpt4all", LLMODEL_PATH)).replace("\\", "\\\\")


def load_llmodel_library():
    system = platform.system()

    def get_c_shared_lib_extension():
        if system == "Darwin":
            return "dylib"
        elif system == "Linux":
            return "so"
        elif system == "Windows":
            return "dll"
        else:
            raise Exception("Operating System not supported")

    c_lib_ext = get_c_shared_lib_extension()

    llmodel_file = "libllmodel" + "." + c_lib_ext

    llmodel_dir = str(pkg_resources.resource_filename("gpt4all", os.path.join(LLMODEL_PATH, llmodel_file))).replace(
        "\\", "\\\\"
    )

    llmodel_lib = ctypes.CDLL(llmodel_dir)

    return llmodel_lib


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

llmodel.llmodel_set_implementation_search_path(MODEL_LIB_PATH.encode("utf-8"))


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
        self.model = llmodel.llmodel_model_create(model_path_enc)

        if self.model is not None:
            llmodel.llmodel_loadModel(self.model, model_path_enc)
        else:
            raise ValueError("Unable to instantiate model")

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
