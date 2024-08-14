from __future__ import annotations

import ctypes
import os
import platform
import re
import subprocess
import sys
import textwrap
import threading
from enum import Enum
from queue import Queue
from typing import TYPE_CHECKING, Any, Callable, Generic, Iterable, Literal, NoReturn, TypeVar, overload

if sys.version_info >= (3, 9):
    import importlib.resources as importlib_resources
else:
    import importlib_resources

if (3, 9) <= sys.version_info < (3, 11):
    # python 3.9 broke generic TypedDict, python 3.11 fixed it
    from typing_extensions import TypedDict
else:
    from typing import TypedDict

if TYPE_CHECKING:
    from typing_extensions import TypeAlias

EmbeddingsType = TypeVar('EmbeddingsType', bound='list[Any]')


# Detect Rosetta 2
if platform.system() == "Darwin" and platform.processor() == "i386":
    if subprocess.run(
        "sysctl -n sysctl.proc_translated".split(), check=True, capture_output=True, text=True,
    ).stdout.strip() == "1":
        raise RuntimeError(textwrap.dedent("""\
            Running GPT4All under Rosetta is not supported due to CPU feature requirements.
            Please install GPT4All in an environment that uses a native ARM64 Python interpreter.
        """))


def _load_cuda(rtver: str, blasver: str) -> None:
    if platform.system() == "Linux":
        cudalib   = f"lib/libcudart.so.{rtver}"
        cublaslib = f"lib/libcublas.so.{blasver}"
    else:  # Windows
        cudalib   = fr"bin\cudart64_{rtver.replace('.', '')}.dll"
        cublaslib = fr"bin\cublas64_{blasver}.dll"

    # preload the CUDA libs so the backend can find them
    ctypes.CDLL(os.path.join(cuda_runtime.__path__[0], cudalib), mode=ctypes.RTLD_GLOBAL)
    ctypes.CDLL(os.path.join(cublas.__path__[0], cublaslib), mode=ctypes.RTLD_GLOBAL)


# Find CUDA libraries from the official packages
cuda_found = False
if platform.system() in ("Linux", "Windows"):
    try:
        from nvidia import cuda_runtime, cublas
    except ImportError:
        pass  # CUDA is optional
    else:
        for rtver, blasver in [("12", "12"), ("11.0", "11")]:
            try:
                _load_cuda(rtver, blasver)
                cuda_found = True
            except OSError:  # dlopen() does not give specific error codes
                pass  # try the next one


# TODO: provide a config file to make this more robust
MODEL_LIB_PATH = importlib_resources.files("gpt4all") / "llmodel_DO_NOT_MODIFY" / "build"


def load_llmodel_library():
    ext = {"Darwin": "dylib", "Linux": "so", "Windows": "dll"}[platform.system()]

    try:
        # macOS, Linux, MinGW
        lib = ctypes.CDLL(str(MODEL_LIB_PATH / f"libllmodel.{ext}"))
    except FileNotFoundError:
        if ext != 'dll':
            raise
        # MSVC
        lib = ctypes.CDLL(str(MODEL_LIB_PATH / "llmodel.dll"))

    return lib


llmodel = load_llmodel_library()


class LLModelPromptContext(ctypes.Structure):
    _fields_ = [
        ("tokens", ctypes.POINTER(ctypes.c_int32)),
        ("tokens_size", ctypes.c_size_t),
        ("n_past", ctypes.c_int32),
        ("n_ctx", ctypes.c_int32),
        ("n_predict", ctypes.c_int32),
        ("top_k", ctypes.c_int32),
        ("top_p", ctypes.c_float),
        ("min_p", ctypes.c_float),
        ("temp", ctypes.c_float),
        ("n_batch", ctypes.c_int32),
        ("repeat_penalty", ctypes.c_float),
        ("repeat_last_n", ctypes.c_int32),
        ("context_erase", ctypes.c_float),
    ]

class LLModelGPUDevice(ctypes.Structure):
    _fields_ = [
        ("backend", ctypes.c_char_p),
        ("index", ctypes.c_int32),
        ("type", ctypes.c_int32),
        ("heapSize", ctypes.c_size_t),
        ("name", ctypes.c_char_p),
        ("vendor", ctypes.c_char_p),
    ]

# Define C function signatures using ctypes
llmodel.llmodel_model_create.argtypes = [ctypes.c_char_p]
llmodel.llmodel_model_create.restype = ctypes.c_void_p

llmodel.llmodel_model_create2.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]
llmodel.llmodel_model_create2.restype = ctypes.c_void_p

llmodel.llmodel_model_destroy.argtypes = [ctypes.c_void_p]
llmodel.llmodel_model_destroy.restype = None

llmodel.llmodel_loadModel.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
llmodel.llmodel_loadModel.restype = ctypes.c_bool
llmodel.llmodel_required_mem.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
llmodel.llmodel_required_mem.restype = ctypes.c_size_t
llmodel.llmodel_isModelLoaded.argtypes = [ctypes.c_void_p]
llmodel.llmodel_isModelLoaded.restype = ctypes.c_bool

PromptCallback = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_int32)
ResponseCallback = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_int32, ctypes.c_char_p)
EmbCancelCallback = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.POINTER(ctypes.c_uint), ctypes.c_uint, ctypes.c_char_p)

llmodel.llmodel_prompt.argtypes = [
    ctypes.c_void_p,
    ctypes.c_char_p,
    ctypes.c_char_p,
    PromptCallback,
    ResponseCallback,
    ctypes.c_bool,
    ctypes.POINTER(LLModelPromptContext),
    ctypes.c_bool,
    ctypes.c_char_p,
]

llmodel.llmodel_prompt.restype = None

llmodel.llmodel_embed.argtypes = [
    ctypes.c_void_p,
    ctypes.POINTER(ctypes.c_char_p),
    ctypes.POINTER(ctypes.c_size_t),
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_size_t),
    ctypes.c_bool,
    ctypes.c_bool,
    EmbCancelCallback,
    ctypes.POINTER(ctypes.c_char_p),
]

llmodel.llmodel_embed.restype = ctypes.POINTER(ctypes.c_float)

llmodel.llmodel_free_embedding.argtypes = [ctypes.POINTER(ctypes.c_float)]
llmodel.llmodel_free_embedding.restype = None

llmodel.llmodel_setThreadCount.argtypes = [ctypes.c_void_p, ctypes.c_int32]
llmodel.llmodel_setThreadCount.restype = None

llmodel.llmodel_set_implementation_search_path.argtypes = [ctypes.c_char_p]
llmodel.llmodel_set_implementation_search_path.restype = None

llmodel.llmodel_threadCount.argtypes = [ctypes.c_void_p]
llmodel.llmodel_threadCount.restype = ctypes.c_int32

llmodel.llmodel_set_implementation_search_path(str(MODEL_LIB_PATH).encode())

llmodel.llmodel_available_gpu_devices.argtypes = [ctypes.c_size_t, ctypes.POINTER(ctypes.c_int32)]
llmodel.llmodel_available_gpu_devices.restype = ctypes.POINTER(LLModelGPUDevice)

llmodel.llmodel_gpu_init_gpu_device_by_string.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_char_p]
llmodel.llmodel_gpu_init_gpu_device_by_string.restype = ctypes.c_bool

llmodel.llmodel_gpu_init_gpu_device_by_struct.argtypes = [ctypes.c_void_p, ctypes.POINTER(LLModelGPUDevice)]
llmodel.llmodel_gpu_init_gpu_device_by_struct.restype = ctypes.c_bool

llmodel.llmodel_gpu_init_gpu_device_by_int.argtypes = [ctypes.c_void_p, ctypes.c_int32]
llmodel.llmodel_gpu_init_gpu_device_by_int.restype = ctypes.c_bool

llmodel.llmodel_model_backend_name.argtypes = [ctypes.c_void_p]
llmodel.llmodel_model_backend_name.restype = ctypes.c_char_p

llmodel.llmodel_model_gpu_device_name.argtypes = [ctypes.c_void_p]
llmodel.llmodel_model_gpu_device_name.restype = ctypes.c_char_p

ResponseCallbackType = Callable[[int, str], bool]
RawResponseCallbackType = Callable[[int, bytes], bool]
EmbCancelCallbackType: TypeAlias = 'Callable[[list[int], str], bool]'


def empty_response_callback(token_id: int, response: str) -> bool:
    return True


# Symbol to terminate from generator
class Sentinel(Enum):
    TERMINATING_SYMBOL = 0


class EmbedResult(Generic[EmbeddingsType], TypedDict):
    embeddings: EmbeddingsType
    n_prompt_tokens: int


class CancellationError(Exception):
    """raised when embedding is canceled"""


class LLModel:
    """
    Base class and universal wrapper for GPT4All language models
    built around llmodel C-API.

    Parameters
    ----------
    model_path : str
        Path to the model.
    n_ctx : int
        Maximum size of context window
    ngl : int
        Number of GPU layers to use (Vulkan)
    backend : str
        Backend to use. One of 'auto', 'cpu', 'metal', 'kompute', or 'cuda'.
    """

    def __init__(self, model_path: str, n_ctx: int, ngl: int, backend: str):
        self.model_path = model_path.encode()
        self.n_ctx = n_ctx
        self.ngl = ngl
        self.context: LLModelPromptContext | None = None
        self.buffer = bytearray()
        self.buff_expecting_cont_bytes: int = 0

        # Construct a model implementation
        err = ctypes.c_char_p()
        model = llmodel.llmodel_model_create2(self.model_path, backend.encode(), ctypes.byref(err))
        if model is None:
            s = err.value
            errmsg = 'null' if s is None else s.decode()

            if (
                backend == 'cuda'
                and not cuda_found
                and errmsg.startswith('Could not find any implementations for backend')
            ):
                print('WARNING: CUDA runtime libraries not found. Try `pip install "gpt4all[cuda]"`\n', file=sys.stderr)

            raise RuntimeError(f"Unable to instantiate model: {errmsg}")
        self.model: ctypes.c_void_p | None = model

    def __del__(self, llmodel=llmodel):
        if hasattr(self, 'model'):
            self.close()

    def close(self) -> None:
        if self.model is not None:
            llmodel.llmodel_model_destroy(self.model)
            self.model = None

    def _raise_closed(self) -> NoReturn:
        raise ValueError("Attempted operation on a closed LLModel")

    @property
    def backend(self) -> Literal["cpu", "kompute", "cuda", "metal"]:
        if self.model is None:
            self._raise_closed()
        return llmodel.llmodel_model_backend_name(self.model).decode()

    @property
    def device(self) -> str | None:
        if self.model is None:
            self._raise_closed()
        dev = llmodel.llmodel_model_gpu_device_name(self.model)
        return None if dev is None else dev.decode()

    @staticmethod
    def list_gpus(mem_required: int = 0) -> list[str]:
        """
        List the names of the available GPU devices with at least `mem_required` bytes of VRAM.

        Args:
            mem_required: The minimum amount of VRAM, in bytes

        Returns:
            A list of strings representing the names of the available GPU devices.
        """
        num_devices = ctypes.c_int32(0)
        devices_ptr = llmodel.llmodel_available_gpu_devices(mem_required, ctypes.byref(num_devices))
        if not devices_ptr:
            raise ValueError("Unable to retrieve available GPU devices")
        return [f'{d.backend.decode()}:{d.name.decode()}' for d in devices_ptr[:num_devices.value]]

    def init_gpu(self, device: str):
        if self.model is None:
            self._raise_closed()

        mem_required = llmodel.llmodel_required_mem(self.model, self.model_path, self.n_ctx, self.ngl)

        if llmodel.llmodel_gpu_init_gpu_device_by_string(self.model, mem_required, device.encode()):
            return

        all_gpus = self.list_gpus()
        available_gpus = self.list_gpus(mem_required)
        unavailable_gpus = [g for g in all_gpus if g not in available_gpus]

        error_msg = (f"Unable to initialize model on GPU: {device!r}" +
                     f"\nAvailable GPUs: {available_gpus}")
        if unavailable_gpus:
            error_msg += f"\nUnavailable GPUs due to insufficient memory: {unavailable_gpus}"
        raise ValueError(error_msg)

    def load_model(self) -> bool:
        """
        Load model from a file.

        Returns
        -------
        True if model loaded successfully, False otherwise
        """
        if self.model is None:
            self._raise_closed()

        return llmodel.llmodel_loadModel(self.model, self.model_path, self.n_ctx, self.ngl)

    def set_thread_count(self, n_threads):
        if self.model is None:
            self._raise_closed()
        if not llmodel.llmodel_isModelLoaded(self.model):
            raise Exception("Model not loaded")
        llmodel.llmodel_setThreadCount(self.model, n_threads)

    def thread_count(self):
        if self.model is None:
            self._raise_closed()
        if not llmodel.llmodel_isModelLoaded(self.model):
            raise Exception("Model not loaded")
        return llmodel.llmodel_threadCount(self.model)

    def _set_context(
        self,
        n_predict: int = 4096,
        top_k: int = 40,
        top_p: float = 0.9,
        min_p: float = 0.0,
        temp: float = 0.1,
        n_batch: int = 8,
        repeat_penalty: float = 1.2,
        repeat_last_n: int = 10,
        context_erase: float = 0.75,
        reset_context: bool = False,
    ):
        if self.context is None:
            context = LLModelPromptContext(
                tokens_size=0,
                n_past=0,
                n_ctx=0,
                n_predict=n_predict,
                top_k=top_k,
                top_p=top_p,
                min_p=min_p,
                temp=temp,
                n_batch=n_batch,
                repeat_penalty=repeat_penalty,
                repeat_last_n=repeat_last_n,
                context_erase=context_erase,
            )
            self.context = context
        else:
            context = self.context
            if reset_context:
                self.context.n_past = 0

        self.context.n_predict = n_predict
        self.context.top_k = top_k
        self.context.top_p = top_p
        self.context.min_p = min_p
        self.context.temp = temp
        self.context.n_batch = n_batch
        self.context.repeat_penalty = repeat_penalty
        self.context.repeat_last_n = repeat_last_n
        self.context.context_erase = context_erase

    @overload
    def generate_embeddings(
        self, text: str, prefix: str | None, dimensionality: int, do_mean: bool, atlas: bool,
        cancel_cb: EmbCancelCallbackType | None,
    ) -> EmbedResult[list[float]]: ...
    @overload
    def generate_embeddings(
        self, text: list[str], prefix: str | None, dimensionality: int, do_mean: bool, atlas: bool,
        cancel_cb: EmbCancelCallbackType | None,
    ) -> EmbedResult[list[list[float]]]: ...
    @overload
    def generate_embeddings(
        self, text: str | list[str], prefix: str | None, dimensionality: int, do_mean: bool, atlas: bool,
        cancel_cb: EmbCancelCallbackType | None,
    ) -> EmbedResult[list[Any]]: ...

    def generate_embeddings(
        self, text: str | list[str], prefix: str | None, dimensionality: int, do_mean: bool, atlas: bool,
        cancel_cb: EmbCancelCallbackType | None,
    ) -> EmbedResult[list[Any]]:
        if not text:
            raise ValueError("text must not be None or empty")

        if self.model is None:
            self._raise_closed()

        if single_text := isinstance(text, str):
            text = [text]

        # prepare input
        embedding_size = ctypes.c_size_t()
        token_count = ctypes.c_size_t()
        error = ctypes.c_char_p()
        c_prefix = ctypes.c_char_p() if prefix is None else prefix.encode()
        c_texts = (ctypes.c_char_p * (len(text) + 1))()
        for i, t in enumerate(text):
            c_texts[i] = t.encode()

        def wrap_cancel_cb(batch_sizes: Any, n_batch: int, backend: bytes) -> bool:
            assert cancel_cb is not None
            return cancel_cb(batch_sizes[:n_batch], backend.decode())

        cancel_cb_wrapper = EmbCancelCallback() if cancel_cb is None else EmbCancelCallback(wrap_cancel_cb)

        # generate the embeddings
        embedding_ptr = llmodel.llmodel_embed(
            self.model, c_texts, ctypes.byref(embedding_size), c_prefix, dimensionality, ctypes.byref(token_count),
            do_mean, atlas, cancel_cb_wrapper, ctypes.byref(error),
        )

        if not embedding_ptr:
            msg = "(unknown error)" if error.value is None else error.value.decode()
            if msg == "operation was canceled":
                raise CancellationError(msg)
            raise RuntimeError(f'Failed to generate embeddings: {msg}')

        # extract output
        n_embd = embedding_size.value // len(text)
        embedding_array = [
            embedding_ptr[i:i + n_embd]
            for i in range(0, embedding_size.value, n_embd)
        ]
        llmodel.llmodel_free_embedding(embedding_ptr)

        embeddings = embedding_array[0] if single_text else embedding_array
        return {'embeddings': embeddings, 'n_prompt_tokens': token_count.value}

    def prompt_model(
        self,
        prompt: str,
        prompt_template: str,
        callback: ResponseCallbackType,
        n_predict: int = 4096,
        top_k: int = 40,
        top_p: float = 0.9,
        min_p: float = 0.0,
        temp: float = 0.1,
        n_batch: int = 8,
        repeat_penalty: float = 1.2,
        repeat_last_n: int = 10,
        context_erase: float = 0.75,
        reset_context: bool = False,
        special: bool = False,
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

        if self.model is None:
            self._raise_closed()

        self.buffer.clear()
        self.buff_expecting_cont_bytes = 0

        self._set_context(
            n_predict=n_predict,
            top_k=top_k,
            top_p=top_p,
            min_p=min_p,
            temp=temp,
            n_batch=n_batch,
            repeat_penalty=repeat_penalty,
            repeat_last_n=repeat_last_n,
            context_erase=context_erase,
            reset_context=reset_context,
        )

        llmodel.llmodel_prompt(
            self.model,
            ctypes.c_char_p(prompt.encode()),
            ctypes.c_char_p(prompt_template.encode()),
            PromptCallback(self._prompt_callback),
            ResponseCallback(self._callback_decoder(callback)),
            True,
            self.context,
            special,
            ctypes.c_char_p(),
        )


    def prompt_model_streaming(
        self, prompt: str, prompt_template: str, callback: ResponseCallbackType = empty_response_callback, **kwargs
    ) -> Iterable[str]:
        if self.model is None:
            self._raise_closed()

        output_queue: Queue[str | Sentinel] = Queue()

        # Put response tokens into an output queue
        def _generator_callback_wrapper(callback: ResponseCallbackType) -> ResponseCallbackType:
            def _generator_callback(token_id: int, response: str):
                nonlocal callback

                if callback(token_id, response):
                    output_queue.put(response)
                    return True

                return False

            return _generator_callback

        def run_llmodel_prompt(prompt: str, prompt_template: str, callback: ResponseCallbackType, **kwargs):
            self.prompt_model(prompt, prompt_template, callback, **kwargs)
            output_queue.put(Sentinel.TERMINATING_SYMBOL)

        # Kick off llmodel_prompt in separate thread so we can return generator
        # immediately
        thread = threading.Thread(
            target=run_llmodel_prompt,
            args=(prompt, prompt_template, _generator_callback_wrapper(callback)),
            kwargs=kwargs,
        )
        thread.start()

        # Generator
        while True:
            response = output_queue.get()
            if isinstance(response, Sentinel):
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
                        decoded.append(self.buffer.decode(errors='replace'))

                        self.buffer.clear()

                    self.buffer.append(byte)
                    self.buff_expecting_cont_bytes = max(0, len(high_ones) - 1)

                if self.buff_expecting_cont_bytes <= 0: 
                    # received the whole sequence or an out of place continuation byte
                    decoded.append(self.buffer.decode(errors='replace'))

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
