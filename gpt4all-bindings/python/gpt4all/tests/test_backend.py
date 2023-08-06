"""
Tests for the GPT4All backend through ctypes.
"""
import ctypes
import os
import sys

import pytest

from gpt4all import pyllmodel
from gpt4all.pyllmodel import LLModelError, LLModelPromptContext, llmodel

from . import get_model_folder


# Windows has more graceful ctypes error handling; some things need to be conditioned on that:
IS_WINDOWS: bool = sys.platform == 'win32'


def test_llmodel_error():
    # pyllmodel.LLModelError
    error = LLModelError()
    # fields test:
    assert [field[0] for field in error._fields_] == ['message', 'code']  # all names
    # testing the Python side of the ctypes instance:
    assert hasattr(error, 'message')
    assert error.message is None  # default value
    error.message = b'error'  # can set bytes
    assert error.message == b'error'
    with pytest.raises(TypeError):
        error.message = 'error'  # cannot set a string
    assert hasattr(error, 'code')
    assert isinstance(error.code, int)
    assert error.code is 0  # default value


def test_llmodel_prompt_context():
    # pyllmodel.LLModelPromptContext
    context = LLModelPromptContext()
    # fields test:
    all_field_names = [
        'logits',
        'logits_size',
        'tokens',
        'tokens_size',
        'n_past',
        'n_ctx',
        'n_predict',
        'top_k',
        'top_p',
        'temp',
        'n_batch',
        'repeat_penalty',
        'repeat_last_n',
        'context_erase',
    ]
    assert [field[0] for field in context._fields_] == all_field_names
    # testing the Python side of the ctypes instance:
    # logits:
    assert isinstance(context.logits, ctypes.POINTER(ctypes.c_float))
    assert not context.logits  # NULL pointer by default, but not mapped to None
    assert context.logits is not None
    # tokens:
    assert isinstance(context.tokens, ctypes.POINTER(ctypes.c_int32))
    assert not context.tokens  # NULL pointer by default, but not mapped to None
    assert context.logits is not None
    # other fields:
    for name in ['logits_size', 'tokens_size', 'n_past', 'n_ctx', 'n_predict', 'top_k', 'n_batch', 'repeat_last_n']:
        assert hasattr(context, name)
        assert isinstance(getattr(context, name), int)
        assert getattr(context, name) == 0  # default value
    for name in ['top_p', 'temp', 'repeat_penalty', 'context_erase']:
        assert hasattr(context, name)
        assert isinstance(getattr(context, name), float)
        assert getattr(context, name) == 0.0  # default value


def model_path_str(model_name: str) -> str:
    if not model_name:
        raise ValueError(f"Invalid model name '{model_name}'")
    folder_path = get_model_folder(model_name)
    model_path = folder_path / model_name
    if not model_path.is_file() or not os.access(model_path, os.R_OK):
        raise RuntimeError(f"Model cannot be accessed '{model_path}'")
    return str(model_path)


@pytest.fixture
def orca_mini_path_str() -> str:
    return model_path_str('orca-mini-3b.ggmlv3.q4_0.bin')


@pytest.fixture
def embeddings_path_str() -> str:
    return model_path_str('ggml-all-MiniLM-L6-v2-f16.bin')


@pytest.mark.c_api_call('llmodel_model_create')
def test_llmodel_model_create(orca_mini_path_str, embeddings_path_str):
    # pyllmodel.llmodel.llmodel_model_create
    if IS_WINDOWS:  # TODO disabling a few for now because a test on Mac caused a segmentation fault
        # TODO even on Windows, this doesn't work reliably
        # model = llmodel.llmodel_model_create(None)  # NULL
        # assert model is None
        model = llmodel.llmodel_model_create(b'')  # empty string
        assert model is None
        model = llmodel.llmodel_model_create(b'  \t \n ')  # whitespace
        assert model is None
    model = llmodel.llmodel_model_create(orca_mini_path_str.encode('utf-8'))
    assert model is not None
    llmodel.llmodel_model_destroy(model)
    model = None
    model = llmodel.llmodel_model_create(embeddings_path_str.encode('utf-8'))
    assert model is not None
    try:  # TODO embeddings model currently buggy; remove try block when fixed
        if IS_WINDOWS:
            llmodel.llmodel_model_destroy(model)
    except:
        pass


@pytest.mark.c_api_call('llmodel_model_create2')
def test_llmodel_model_create2(orca_mini_path_str, embeddings_path_str):
    # pyllmodel.llmodel.llmodel_model_create2
    build_variant = b'auto'
    error = LLModelError()
    def reset(error):
        error.code = 0
        error.message = None
    if IS_WINDOWS:  # TODO disabling a few for now because a test on Mac caused a segmentation fault
        # TODO even on Windows, this doesn't work reliably
        # model = llmodel.llmodel_model_create2(None, build_variant, error)  # NULL
        # assert model is None
        # note: slight difference in the error string on different platforms:
        # - Windows MinGW: "basic_string: construction from null is not valid"
        # - Linux:         "basic_string::_M_construct null not valid"
        # assert error.code == 22 and b'null' in error.message and b'not valid' in error.message
        # reset(error); model = None
        model = llmodel.llmodel_model_create2(b'', build_variant, error)  # empty string
        assert model is None
        # note: different error codes/messages on different platforms:
        assert error.code == 2 or error.code == 22
        assert error.message == b'No such file or directory' or error.message == b'Invalid argument'
        reset(error); model = None
        model = llmodel.llmodel_model_create2(b'  \t \n ', build_variant, error)  # whitespace
        assert model is None
        # note: different error codes/messages on different platforms:
        assert error.code == 2 or error.code == 22
        assert error.message == b'No such file or directory' or error.message == b'Invalid argument'
        reset(error); model = None
    model = llmodel.llmodel_model_create2(orca_mini_path_str.encode('utf-8'), build_variant, error)
    assert model is not None  # address pointer, so the value can vary
    assert error.code == 0 and error.message is None
    llmodel.llmodel_model_destroy(model)
    reset(error); model = None
    model = llmodel.llmodel_model_create2(embeddings_path_str.encode('utf-8'), build_variant, error)
    assert model is not None  # address pointer, so the value can vary
    assert error.code == 0 and error.message is None
    try:  # TODO embeddings model currently buggy; remove try block when fixed
        if IS_WINDOWS:
            llmodel.llmodel_model_destroy(model)
    except:
        pass


@pytest.mark.c_api_call('llmodel_model_destroy')
def test_llmodel_model_destroy(orca_mini_path_str):
    # pyllmodel.llmodel.llmodel_model_destroy
    llmodel.llmodel_model_destroy(None)
    build_variant = b'auto'
    error = LLModelError()
    model = llmodel.llmodel_model_create2(orca_mini_path_str.encode('utf-8'), build_variant, error)
    assert model is not None
    llmodel.llmodel_model_destroy(model)
    # TODO this doesn't work reliably
    # if IS_WINDOWS:
    #     with pytest.raises(OSError):
    #         llmodel.llmodel_model_destroy(model)  # repeat calls are not ok


@pytest.fixture
def orca_mini_model(orca_mini_path_str) -> ctypes.c_void_p:
    build_variant, error = b'auto', LLModelError()
    model = llmodel.llmodel_model_create2(orca_mini_path_str.encode('utf-8'), build_variant, error)
    if error.code != 0:
        raise RuntimeError(f"Failed to create '{orca_mini_path}'; code: {error.code}; message: {error.message}")
    yield model
    if model is not None:
        llmodel.llmodel_model_destroy(model)


@pytest.fixture
def embeddings_model(embeddings_path_str) -> ctypes.c_void_p:
    build_variant, error = b'auto', LLModelError()
    model = llmodel.llmodel_model_create2(embeddings_path_str.encode('utf-8'), build_variant, error)
    if error.code != 0:
        raise RuntimeError(f"Failed to create '{embeddings_path}'; code: {error.code}; message: {error.message}")
    yield model
    try:  # TODO embeddings model currently buggy; remove try block when fixed
        if IS_WINDOWS:
            llmodel.llmodel_model_destroy(model)
    except:
        pass


@pytest.mark.c_api_call('llmodel_required_mem')
def test_llmodel_required_mem(orca_mini_path_str, embeddings_path_str):
    # pyllmodel.llmodel.llmodel_required_mem

    def create_model(path):
        build_variant = b'auto'
        error = LLModelError()
        model = llmodel.llmodel_model_create2(path, build_variant, error)
        if error.code != 0:
            raise RuntimeError(f"Failed to create model. Cause: {error.message}")
        return model

    paths = [orca_mini_path_str.encode('utf-8'), embeddings_path_str.encode('utf-8')]
    models = list(map(create_model, paths))
    orca_required_mem = llmodel.llmodel_required_mem(models[0], paths[0])
    assert orca_required_mem == 2_767_307_008
    embeddings_required_mem = llmodel.llmodel_required_mem(models[1], paths[1])
    assert embeddings_required_mem >= 0  # TODO doesn't implement required_mem yet
    for model in models:
        try:  # TODO embeddings model currently buggy; remove try block when fixed
            if IS_WINDOWS:
                llmodel.llmodel_model_destroy(model)
        except:
            pass


@pytest.mark.c_api_call('llmodel_loadModel')
def test_llmodel_loadModel_valid(orca_mini_model, orca_mini_path_str, embeddings_model, embeddings_path_str):
    # pyllmodel.llmodel.llmodel_loadModel
    is_orca_mini_model_loaded = llmodel.llmodel_loadModel(orca_mini_model, orca_mini_path_str.encode('utf-8'))
    assert is_orca_mini_model_loaded
    is_embeddings_model_loaded = llmodel.llmodel_loadModel(embeddings_model, embeddings_path_str.encode('utf-8'))
    assert is_embeddings_model_loaded


@pytest.mark.c_api_call('llmodel_loadModel')
def test_llmodel_loadModel_invalid(orca_mini_model, embeddings_model):
    # pyllmodel.llmodel.llmodel_loadModel
    is_orca_mini_model_loaded = llmodel.llmodel_loadModel(orca_mini_model, 'invalid-model.bin'.encode('utf-8'))
    assert not is_orca_mini_model_loaded


@pytest.mark.c_api_call('llmodel_isModelLoaded')
def test_llmodel_isModelLoaded(orca_mini_path_str):
    # pyllmodel.llmodel.llmodel_isModelLoaded
    build_variant = b'auto'
    error = LLModelError()
    model = llmodel.llmodel_model_create2(orca_mini_path_str.encode('utf-8'), build_variant, error)
    assert error.code == 0
    is_loaded = llmodel.llmodel_isModelLoaded(model)  # not yet
    assert not is_loaded
    is_loaded = llmodel.llmodel_loadModel(model, orca_mini_path_str.encode('utf-8'))
    assert is_loaded
    is_loaded = llmodel.llmodel_isModelLoaded(model)
    assert is_loaded
    llmodel.llmodel_model_destroy(model)


@pytest.mark.c_api_call('llmodel_prompt')
@pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
def test_llmodel_prompt(orca_mini_model, orca_mini_path_str):
    # pyllmodel.llmodel.llmodel_prompt
    is_loaded = llmodel.llmodel_loadModel(orca_mini_model, orca_mini_path_str.encode('utf-8'))
    assert is_loaded
    response_tokens = []

    def response_callback(token_id: int, response: bytes) -> bool:
        response_tokens.append(response.decode('utf-8'))
        return True

    prompt = b'the capital of France is:'
    prompt_callback = pyllmodel.PromptCallback(lambda *_: True)
    response_callback = pyllmodel.ResponseCallback(response_callback)
    recalculate_callback = pyllmodel.RecalculateCallback(lambda *_: True)
    ctx = LLModelPromptContext(temp=0, n_predict=1, n_batch=1)
    llmodel.llmodel_prompt(orca_mini_model, prompt, prompt_callback, response_callback, recalculate_callback, ctx)
    assert ''.join(response_tokens) == ' Paris'


@pytest.mark.c_api_call('llmodel_embedding')
def test_llmodel_embedding(embeddings_model, embeddings_path_str):
    # pyllmodel.llmodel.llmodel_embedding
    embedding_size = ctypes.c_size_t()
    is_loaded = llmodel.llmodel_loadModel(embeddings_model, embeddings_path_str.encode('utf-8'))
    assert is_loaded
    embedding_ptr = llmodel.llmodel_embedding(embeddings_model, b'valid string', ctypes.byref(embedding_size))
    assert embedding_ptr is not None  # address pointer, so the value can vary
    assert embedding_size.value == 384
    embedding = [embedding_ptr[i] for i in range(embedding_size.value)]
    assert len(embedding) == 384
    llmodel.llmodel_free_embedding(embedding_ptr)
    assert True  # everything completed without errors


@pytest.mark.c_api_call('llmodel_setThreadCount', 'llmodel_threadCount')
@pytest.mark.parametrize('n_threads', [0, 1, 2, 4, 8, 65])
def test_llmodel_get_set_threadCount_valid(orca_mini_model, n_threads, orca_mini_path_str):
    # pyllmodel.llmodel.llmodel_setThreadCount / pyllmodel.llmodel.llmodel_threadCount
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    assert current_thread_count == 0  # default when not loaded
    llmodel.llmodel_setThreadCount(orca_mini_model, n_threads)
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    assert current_thread_count == n_threads
    # reset thread count to test on loaded model:
    llmodel.llmodel_setThreadCount(orca_mini_model, 0)
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    assert current_thread_count == 0  # default again
    is_loaded = llmodel.llmodel_loadModel(orca_mini_model, orca_mini_path_str.encode('utf-8'))
    assert is_loaded
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    print(f"Default thread count on this machine is: {current_thread_count}")
    assert 0 <= current_thread_count <= 4  # default is system dependent API caps the number at 4 threads
    llmodel.llmodel_setThreadCount(orca_mini_model, n_threads)
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    assert current_thread_count == n_threads


@pytest.mark.c_api_call('llmodel_setThreadCount', 'llmodel_threadCount')
@pytest.mark.parametrize('n_threads', [None, b'10011', 'sixteen'])
def test_llmodel_get_set_threadCount_exception(orca_mini_model, n_threads):
    # pyllmodel.llmodel.llmodel_setThreadCount / pyllmodel.llmodel.llmodel_threadCount
    if IS_WINDOWS:
        with pytest.raises(OSError):
            llmodel.llmodel_setThreadCount(None, 1)  # access violation
        with pytest.raises(OSError):
            llmodel.llmodel_threadCount(None)  # access violation
    with pytest.raises(Exception):
        llmodel.llmodel_setThreadCount(orca_mini_model, n_threads)


@pytest.mark.c_api_call('llmodel_setThreadCount', 'llmodel_threadCount')
@pytest.mark.parametrize('n_threads', [-1, -4, -257])
def test_llmodel_get_set_threadCount_invalid(orca_mini_model, n_threads, orca_mini_path_str):
    # pyllmodel.llmodel.llmodel_setThreadCount / pyllmodel.llmodel.llmodel_threadCount
    # note: negative values make no sense but are allowed by the API regardless
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    assert current_thread_count == 0  # default when not loaded
    llmodel.llmodel_setThreadCount(orca_mini_model, n_threads)
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    assert current_thread_count == n_threads
    # reset thread count to test on loaded model:
    llmodel.llmodel_setThreadCount(orca_mini_model, 0)
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    assert current_thread_count == 0  # default again
    is_loaded = llmodel.llmodel_loadModel(orca_mini_model, orca_mini_path_str.encode('utf-8'))
    assert is_loaded
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    print(f"Default thread count on this machine is: {current_thread_count}")
    assert 0 <= current_thread_count <= 4  # default is system dependent; API caps the number at 4 threads
    llmodel.llmodel_setThreadCount(orca_mini_model, n_threads)
    current_thread_count = llmodel.llmodel_threadCount(orca_mini_model)
    assert current_thread_count == n_threads
