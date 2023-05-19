import pkg_resources
import ctypes
import os
import platform
import re
import subprocess
import sys

class DualStreamProcessor:
    def __init__(self, stream=None):
        self.stream = stream
        self.output = ""

    def write(self, text):
        cleaned_text = re.sub(r"\n(?!\n)", "", text)
        if self.stream is not None:
            self.stream.write(cleaned_text)
            self.stream.flush()
        self.output += cleaned_text


# TODO: provide a config file to make this more robust
LLMODEL_PATH = os.path.join("llmodel_DO_NOT_MODIFY", "build").replace("\\", "\\\\")

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

    llmodel_file = "libllmodel" + '.' + c_lib_ext
    llama_file = "libllama" + '.' + c_lib_ext
    llama_dir = str(pkg_resources.resource_filename('gpt4all', os.path.join(LLMODEL_PATH, llama_file)))
    llmodel_dir = str(pkg_resources.resource_filename('gpt4all', os.path.join(LLMODEL_PATH, llmodel_file)))

    # For windows
    llama_dir = llama_dir.replace("\\", "\\\\")
    llmodel_dir = llmodel_dir.replace("\\", "\\\\")

    llama_lib = ctypes.CDLL(llama_dir, mode=ctypes.RTLD_GLOBAL)
    llmodel_lib = ctypes.CDLL(llmodel_dir)

    return llmodel_lib, llama_lib


llmodel, llama = load_llmodel_library()

# Define C function signatures using ctypes
llmodel.llmodel_gptj_create.restype = ctypes.c_void_p
llmodel.llmodel_gptj_destroy.argtypes = [ctypes.c_void_p]
llmodel.llmodel_llama_create.restype = ctypes.c_void_p
llmodel.llmodel_llama_destroy.argtypes = [ctypes.c_void_p]
llmodel.llmodel_mpt_create.restype = ctypes.c_void_p
llmodel.llmodel_mpt_destroy.argtypes = [ctypes.c_void_p]


llmodel.llmodel_loadModel.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
llmodel.llmodel_loadModel.restype = ctypes.c_bool
llmodel.llmodel_isModelLoaded.argtypes = [ctypes.c_void_p]
llmodel.llmodel_isModelLoaded.restype = ctypes.c_bool

class LLModelPromptContext(ctypes.Structure):
    _fields_ = [("logits", ctypes.POINTER(ctypes.c_float)),
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
                ("context_erase", ctypes.c_float)]
    
ResponseCallback = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_int32, ctypes.c_char_p)
RecalculateCallback = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_bool)

llmodel.llmodel_prompt.argtypes = [ctypes.c_void_p, 
                                   ctypes.c_char_p, 
                                   ResponseCallback, 
                                   ResponseCallback, 
                                   RecalculateCallback, 
                                   ctypes.POINTER(LLModelPromptContext)]

llmodel.llmodel_prompt.restype = None

llmodel.llmodel_setThreadCount.argtypes = [ctypes.c_void_p, ctypes.c_int32]
llmodel.llmodel_setThreadCount.restype = None

llmodel.llmodel_threadCount.argtypes = [ctypes.c_void_p]
llmodel.llmodel_threadCount.restype = ctypes.c_int32


class LLModel:
    """
    Base class and universal wrapper for GPT4All language models
    built around llmodel C-API.

    Attributes
    ----------
    model: llmodel_model
        Ctype pointer to underlying model
    model_type : str
        Model architecture identifier
    """

    model_type: str = None

    def __init__(self):
        self.model = None
        self.model_name = None

    def __del__(self):
        pass

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
        llmodel.llmodel_loadModel(self.model, model_path.encode('utf-8'))
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


    def generate(self, 
                 prompt: str,
                 logits_size: int = 0, 
                 tokens_size: int = 0, 
                 n_past: int = 0, 
                 n_ctx: int = 1024, 
                 n_predict: int = 128, 
                 top_k: int = 40, 
                 top_p: float = .9, 
                 temp: float = .1, 
                 n_batch: int = 8, 
                 repeat_penalty: float = 1.2, 
                 repeat_last_n: int = 10, 
                 context_erase: float = .5,
                 streaming: bool = False) -> str:
        """
        Generate response from model from a prompt.

        Parameters
        ----------
        prompt: str
            Question, task, or conversation for model to respond to
        streaming: bool
            Stream response to stdout

        Returns
        -------
        Model response str
        """
        
        prompt = prompt.encode('utf-8')
        prompt = ctypes.c_char_p(prompt)

        old_stdout = sys.stdout 

        stream_processor = DualStreamProcessor()
    
        if streaming:
            stream_processor.stream = sys.stdout
        
        sys.stdout = stream_processor

        context = LLModelPromptContext(
            logits_size=logits_size, 
            tokens_size=tokens_size, 
            n_past=n_past, 
            n_ctx=n_ctx, 
            n_predict=n_predict, 
            top_k=top_k, 
            top_p=top_p, 
            temp=temp, 
            n_batch=n_batch, 
            repeat_penalty=repeat_penalty, 
            repeat_last_n=repeat_last_n, 
            context_erase=context_erase
        )

        llmodel.llmodel_prompt(self.model, 
                               prompt, 
                               ResponseCallback(self._prompt_callback), 
                               ResponseCallback(self._response_callback), 
                               RecalculateCallback(self._recalculate_callback), 
                               context)

        # Revert to old stdout
        sys.stdout = old_stdout
        # Force new line
        print()

        return stream_processor.output

    # Empty prompt callback
    @staticmethod
    def _prompt_callback(token_id, response):
        return True

    # Empty response callback method that just prints response to be collected
    @staticmethod
    def _response_callback(token_id, response):
        print(response.decode('utf-8'))
        return True

    # Empty recalculate callback
    @staticmethod
    def _recalculate_callback(is_recalculating):
        return is_recalculating


class GPTJModel(LLModel):

    model_type = "gptj"

    def __init__(self):
        super().__init__()
        self.model = llmodel.llmodel_gptj_create()

    def __del__(self):
        if self.model is not None and llmodel is not None:
            llmodel.llmodel_gptj_destroy(self.model)
        super().__del__()


class LlamaModel(LLModel):

    model_type = "llama"

    def __init__(self):
        super().__init__()
        self.model = llmodel.llmodel_llama_create()

    def __del__(self):
        if self.model is not None and llmodel is not None:
            llmodel.llmodel_llama_destroy(self.model)
        super().__del__()


class MPTModel(LLModel):

    model_type = "mpt"

    def __init__(self):
        super().__init__()
        self.model = llmodel.llmodel_mpt_create()

    def __del__(self):
        if self.model is not None and llmodel is not None:
            llmodel.llmodel_mpt_destroy(self.model)
        super().__del__()
