import ctypes
import os
import sys

# Load the shared library
if sys.platform == "darwin":
    lib_ext = ".dylib"
else:
    lib_ext = ".so"

_lib_path = os.path.join(
    os.path.dirname(__file__), 
    "../../",
    "gpt4all-backend",
    "build",
    f"libllmodel{lib_ext}"
) 


_lib = ctypes.CDLL(_lib_path)


# Define the data structures
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

# Set the argument types and return types of the functions
_lib.llmodel_gptj_create.argtypes = []
_lib.llmodel_gptj_create.restype = ctypes.c_void_p

_lib.llmodel_gptj_destroy.argtypes = [ctypes.c_void_p]
_lib.llmodel_gptj_destroy.restype = None

# Define the remaining functions and their argument types and return types
_lib.llmodel_mpt_create.argtypes = []
_lib.llmodel_mpt_create.restype = ctypes.c_void_p

_lib.llmodel_mpt_destroy.argtypes = [ctypes.c_void_p]
_lib.llmodel_mpt_destroy.restype = None

_lib.llmodel_llama_create.argtypes = []
_lib.llmodel_llama_create.restype = ctypes.c_void_p

_lib.llmodel_llama_destroy.argtypes = [ctypes.c_void_p]
_lib.llmodel_llama_destroy.restype = None

_lib.llmodel_loadModel.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_lib.llmodel_loadModel.restype = ctypes.c_bool

_lib.llmodel_isModelLoaded.argtypes = [ctypes.c_void_p]
_lib.llmodel_isModelLoaded.restype = ctypes.c_bool

_lib.llmodel_get_state_size.argtypes = [ctypes.c_void_p]
_lib.llmodel_get_state_size.restype = ctypes.c_uint64

_lib.llmodel_save_state_data.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8)]
_lib.llmodel_save_state_data.restype = ctypes.c_uint64

_lib.llmodel_restore_state_data.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8)]
_lib.llmodel_restore_state_data.restype = ctypes.c_uint64

_lib.llmodel_prompt.argtypes = [ctypes.c_void_p, ctypes.c_char_p,
                                ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_int32),
                                ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_int32, ctypes.c_char_p),
                                ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_bool),
                                ctypes.POINTER(LLModelPromptContext)]

_lib.llmodel_prompt.restype = None

_lib.llmodel_setThreadCount.argtypes = [ctypes.c_void_p, ctypes.c_int32]
_lib.llmodel_setThreadCount.restype = None

_lib.llmodel_threadCount.argtypes = [ctypes.c_void_p]
_lib.llmodel_threadCount.restype = ctypes.c_int32

def llmodel_gptj_create():
    return _lib.llmodel_gptj_create()

def llmodel_gptj_destroy(gptj):
    _lib.llmodel_gptj_destroy(gptj)

def llmodel_mpt_create():
    return _lib.llmodel_mpt_create()

def llmodel_mpt_destroy(mpt):
    _lib.llmodel_mpt_destroy(mpt)

def llmodel_llama_create():
    return _lib.llmodel_llama_create()

def llmodel_llama_destroy(llama):
    _lib.llmodel_llama_destroy(llama)

def llmodel_loadModel(model, model_path):
    return _lib.llmodel_loadModel(model, model_path.encode("utf-8"))

def llmodel_isModelLoaded(model):
    return _lib.llmodel_isModelLoaded(model)

def llmodel_get_state_size(model):
    return _lib.llmodel_get_state_size(model)

def llmodel_save_state_data(model, dest):
    return _lib.llmodel_save_state_data(model, dest)

def llmodel_restore_state_data(model, src):
    return _lib.llmodel_restore_state_data(model, src)

def llmodel_prompt(model, prompt, prompt_callback, response_callback, recalculate_callback, ctx):
    PromptCallbackType = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_int32)
    c_prompt_callback = PromptCallbackType(prompt_callback)

    ResponseCallbackType = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_int32, ctypes.c_char_p)
    c_response_callback = ResponseCallbackType(response_callback)

    RecalculateCallbackType = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_bool)
    c_recalculate_callback = RecalculateCallbackType(recalculate_callback)

    _lib.llmodel_prompt(model, prompt.encode("utf-8"), c_prompt_callback, c_response_callback, c_recalculate_callback, ctypes.pointer(ctx))

def llmodel_setThreadCount(model, n_threads):
    _lib.llmodel_setThreadCount(model, n_threads)

def llmodel_threadCount(model):
    return _lib.llmodel_threadCount(model)
