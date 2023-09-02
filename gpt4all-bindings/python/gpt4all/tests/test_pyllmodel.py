"""
Tests for pyllmodel.LLModel specifically, and to a lesser degree the module in general.

This does not include ctypes/backend focused tests. For those, see test_backend.py instead.
"""

import ctypes
import os
import sys
from unittest.mock import Mock

import pytest

from gpt4all import LLModel, pyllmodel

from . import get_model_folder


# Windows has more graceful ctypes error handling; some things need to be conditioned on that:
IS_WINDOWS: bool = sys.platform == 'win32'
# running on Metal seems to cause some inference responses to differ:
IS_MACOS: bool = sys.platform == 'darwin'


def test_llmodel_path():
    # gpt4all.pyllmodel.LLMODEL_PATH
    llmodel_path = pyllmodel.LLMODEL_PATH
    assert isinstance(llmodel_path, str)
    assert 'llmodel_DO_NOT_MODIFY' in llmodel_path
    assert 'build' in llmodel_path


def test_model_lib_path():
    # gpt4all.pyllmodel.MODEL_LIB_PATH
    model_lib_path = pyllmodel.MODEL_LIB_PATH
    assert isinstance(model_lib_path, str)
    assert 'llmodel_DO_NOT_MODIFY' in model_lib_path
    assert 'build' in model_lib_path
    assert os.path.exists(model_lib_path) and os.path.isdir(model_lib_path)


def test_load_llmodel_library():
    llmodel = pyllmodel.load_llmodel_library()
    assert isinstance(llmodel, ctypes.CDLL)
    assert 'llmodel' in llmodel._name


class TestLLModelClass:
    def test_creation(self):
        llmodel = LLModel()
        assert hasattr(llmodel, 'model')
        assert hasattr(llmodel, 'model_name')
        assert hasattr(llmodel, 'context')
        assert hasattr(llmodel, 'llmodel_lib')
        assert hasattr(llmodel, 'buffer')
        assert hasattr(llmodel, 'buff_expecting_cont_bytes')
        assert isinstance(llmodel.llmodel_lib, ctypes.CDLL)
        assert isinstance(llmodel.buffer, bytearray)
        assert isinstance(llmodel.buff_expecting_cont_bytes, int)

    @pytest.mark.parametrize('token_id', [42, 289293, -14, 10e10])  # even nonsensical token IDs
    def test_empty_prompt_callback(self, token_id):
        assert LLModel._prompt_callback(token_id)  # simply always True

    def test_empty_recalculate_callback(self):
        for should_be_bool in (True, False, None, 20):
            # no effort is made to sanitize the input, user is expected to follow the type hint:
            assert LLModel._recalculate_callback(should_be_bool) is should_be_bool


class TestLLModelMockInstance:
    """
    Mock tests for pyllmodel.LLModel instances

    - main goal here is to focus on the LLModel logic itself; although for simplicity, not all code paths are covered
    - none of the tests require the backend to be compiled, all ctypes calls are replaced with mocks
    - using mocks makes it possible to assert which backend functions were called
    - note: there some are redundancies with other tests outside this class
    """

    @pytest.fixture
    def mock_model(self, monkeypatch):
        # important: with how the mocks are coded, use only one fixture at a time or they'll interfere with each other
        monkeypatch.setattr(pyllmodel, 'llmodel', Mock(spec=pyllmodel.llmodel))
        mock_model = LLModel()
        # set some desired return values and state:
        pyllmodel.llmodel.llmodel_required_mem.return_value = 1_000_000_000
        pyllmodel.llmodel.n_threads = 0
        mock_model.llmodel_lib_is_loaded = False  # True when pretending .llmodel_lib.llmodel_loadModel was called
        # add some mock logic:
        # - load model:
        def llmodel_loadModel(model_path):
            mock_model.model_name = 'mock-model.bin'
            mock_model.llmodel_lib_is_loaded = True
            return True
        pyllmodel.llmodel.llmodel_loadModel.side_effect = lambda _, model_path: llmodel_loadModel(model_path)
        # - thread count:
        def setThreadCount(n_threads):
            if not mock_model.llmodel_lib_is_loaded:
                raise Exception()
            pyllmodel.llmodel.n_threads = n_threads
        def threadCount():
            if not mock_model.llmodel_lib_is_loaded:
                raise Exception()
            return pyllmodel.llmodel.n_threads
        pyllmodel.llmodel.llmodel_setThreadCount.side_effect = lambda _, n_threads: setThreadCount(n_threads)
        pyllmodel.llmodel.llmodel_threadCount.side_effect = lambda _: threadCount()
        # - embedding:
        def llmodel_embedding(c_text, embedding_size):
            embedding_size._obj.value = 384
            return (ctypes.c_float * 384)(*[i / 11 for i in range(384)])  # just some floats, but exactly 384 of them
        pyllmodel.llmodel.llmodel_embedding.side_effect = lambda _, c_text, embedding_size: llmodel_embedding(
            c_text, embedding_size
        )
        # - prompt:
        def llmodel_prompt(prompt_ptr, response_callback):
            for token in "the quick brown fox".encode('utf-8').split():
                response_callback(ctypes.c_int32(0), ctypes.c_char_p(token))  # don't care about token IDs here
        pyllmodel.llmodel.llmodel_prompt.side_effect = (
            lambda _, prompt_ptr, __, response_callback, *___: llmodel_prompt(prompt_ptr, response_callback)
        )
        return mock_model

    @pytest.fixture
    def loaded_mock_model(self, mock_model):
        # important: with how the mocks are coded, use only one fixture at a time or they'll interfere with each other
        mock_model.model = Mock()
        mock_model.model_name = 'mock-model.bin'
        mock_model.llmodel_lib_is_loaded = True
        return mock_model

    def test_memory_needed(self, mock_model):
        name = 'mock-model.bin'  # on the model, the name is only set after creation (load_model)
        folder = str(get_model_folder(name))
        model_path = os.path.join(folder, name)
        memory_needed = mock_model.memory_needed(model_path)
        assert memory_needed == 1_000_000_000  # predefined in fixture
        mock_model.llmodel_lib.llmodel_required_mem.assert_called_once()

    def test_load_model(self, mock_model):
        name = 'mock-model.bin'  # on the model, the name is only set after creation
        folder = str(get_model_folder(name))
        model_path = os.path.join(folder, name)
        is_loaded = mock_model.load_model(model_path)
        assert is_loaded
        mock_model.llmodel_lib.llmodel_loadModel.assert_called_once()
        assert mock_model.model_name in name  # doesn't have the file extension

    def test_set_thread_count_valid(self, loaded_mock_model):
        loaded_mock_model.set_thread_count(9)
        assert loaded_mock_model.thread_count() == 9
        loaded_mock_model.llmodel_lib.llmodel_setThreadCount.assert_called_once()

    def test_set_thread_count_invalid(self, mock_model):
        with pytest.raises(Exception):
            mock_model.set_thread_count(12)  # needs to be loaded

    def test_get_thread_count_valid(self, loaded_mock_model):
        assert loaded_mock_model.thread_count() == 0  # default
        loaded_mock_model.llmodel_lib.llmodel_threadCount.assert_called_once()

    def test_get_thread_count_invalid(self, mock_model):
        with pytest.raises(Exception):
            mock_model.thread_count()  # needs to be loaded

    def test_set_context(self, mock_model):
        assert mock_model.context is None  # starts without a context
        mock_model._set_context()
        assert mock_model.context is not None
        assert isinstance(mock_model.context, pyllmodel.LLModelPromptContext)
        # simulating a reset:
        mock_model.context.n_past = 100  # pretend it has moved 100 tokens in
        mock_model._set_context(reset_context=True)
        assert mock_model.context is not None
        assert isinstance(mock_model.context, pyllmodel.LLModelPromptContext)
        assert mock_model.context.n_past == 0

    def test_generate_embedding_valid(self, loaded_mock_model):
        embedding = loaded_mock_model.generate_embedding("some text")
        loaded_mock_model.llmodel_lib.llmodel_embedding.assert_called_once()
        assert embedding is not None
        assert len(embedding) == 384
        assert all(isinstance(e, float) for e in embedding)

    def test_generate_embedding_invalid(self, mock_model):
        with pytest.raises(ValueError):
            mock_model.generate_embedding(None)  # mustn't be None
        with pytest.raises(ValueError):
            mock_model.generate_embedding("")  # mustn't be empty
        with pytest.raises(RuntimeError):
            mock_model.generate_embedding("some text")  # model not loaded

    def test_prompt_model(self, loaded_mock_model):
        response_tokens = []

        def callback(token_id: int, response: str) -> bool:
            response_tokens.append(response)
            return True

        # note: prompt here is irrelevant, the response is preset in the 'loaded_mock_model'
        loaded_mock_model.prompt_model("who jumped over the lazy dog?", callback)
        pyllmodel.llmodel.llmodel_prompt.assert_called_once()
        assert ' '.join(response_tokens) == "the quick brown fox"

    def test_prompt_model_streaming(self, loaded_mock_model):
        def callback(token_id: int, response: str) -> bool:
            return True

        # note: prompt here is irrelevant, the response is preset in the 'loaded_mock_model'
        generation_iter = loaded_mock_model.prompt_model_streaming("who jumped over the lazy dog?", callback)
        pyllmodel.llmodel.llmodel_prompt.assert_not_called()
        assert ' '.join(generation_iter) == "the quick brown fox"
        pyllmodel.llmodel.llmodel_prompt.assert_called_once()

    @pytest.mark.parametrize(
        'test_generation, test_generation_encoded',
        [
            ("the quick brown fox", b"the quick brown fox"),
            (
                "مرحباً كيف حالك",
                b"\xd9\x85\xd8\xb1\xd8\xad\xd8\xa8\xd8\xa7\xd9\x8b \xd9\x83\xd9\x8a\xd9\x81 \xd8\xad\xd8\xa7\xd9\x84\xd9\x83",
            ),
            (
                "😊 🤔 👨‍💻 🎉 🐶",
                b'\xf0\x9f\x98\x8a \xf0\x9f\xa4\x94 \xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x92\xbb \xf0\x9f\x8e\x89 \xf0\x9f\x90\xb6',
            ),
            (
                "问候 朋友",
                b"\xe9\x97\xae\xe5\x80\x99 \xe6\x9c\x8b\xe5\x8f\x8b",
            ),
        ],
    )
    def test_callback_decoder(self, mock_model, test_generation, test_generation_encoded):
        # there are no backend calls in '_callback_decoder()'; the model holds the bytes buffer for proper decoding
        generation_iter = iter(test_generation.split())
        encoded_generation_iter = iter(test_generation_encoded.split())

        def callback(token_id: int, response: str) -> bool:
            try:
                # test happens here; logic flow is non-obvious, just remember that it goes from bytes -> str
                # the 'response' is originally UTF-8 bytes and must be decoded to the 'test_generation' tokens
                print(response, end=' ')
                assert response == next(generation_iter)
            except StopIteration:
                print()
            return True

        raw_response_callback = mock_model._callback_decoder(callback)
        while True:
            try:
                # this happens first:
                raw_response_callback(0, next(encoded_generation_iter))  # 1. param is 'token_id'; can be ignored here
            except StopIteration:
                break


class TestLLModelInstance:
    @pytest.fixture
    def model(self):
        model = LLModel()
        yield model
        del model  # making sure the memory gets freed asap

    @pytest.fixture
    def loaded_orca_mini_model(self):
        name = 'orca-mini-3b.ggmlv3.q4_0.bin'
        folder = str(get_model_folder(name))
        model_path = os.path.join(folder, name)
        model = LLModel()
        model.load_model(model_path)
        yield model
        del model  # making sure the memory gets freed asap

    @pytest.fixture
    def loaded_embeddings_model(self):
        name = 'ggml-all-MiniLM-L6-v2-f16.bin'
        folder = str(get_model_folder(name))
        model_path = os.path.join(folder, name)
        model = LLModel()
        model.load_model(model_path)
        yield model
        del model  # making sure the memory gets freed asap

    def test_memory_needed(self, model):
        # valid:
        name = 'orca-mini-3b.ggmlv3.q4_0.bin'  # on the model, the name is only set after creation (load_model)
        folder = str(get_model_folder(name))
        model_path = os.path.join(folder, name)
        memory_needed = model.memory_needed(model_path)
        assert memory_needed > 0
        # invalid:
        with pytest.raises(ValueError):
            model_path = os.path.join(folder, 'invalid-model.bin')
            model.memory_needed(model_path)  # ValueError: Unable to instantiate model
        with pytest.raises(AttributeError):
            model.memory_needed(None)  # AttributeError: 'NoneType' object has no attribute 'encode'

    def test_load_model(self, model):
        # valid:
        name = 'orca-mini-3b.ggmlv3.q4_0.bin'  # on the model, the name is only set after creation
        folder = str(get_model_folder(name))
        model_path = os.path.join(folder, name)
        is_loaded = model.load_model(model_path)
        assert is_loaded
        assert model.model_name in name  # doesn't have the file extension
        # invalid:
        with pytest.raises(ValueError):
            model_path = os.path.join(folder, 'invalid-model.bin')
            model.load_model(model_path)  # ValueError: Unable to instantiate model
        with pytest.raises(AttributeError):
            model.load_model(None)  # AttributeError: 'NoneType' object has no attribute 'encode'

    def test_set_thread_count_valid(self, loaded_orca_mini_model):
        loaded_orca_mini_model.set_thread_count(9)
        assert loaded_orca_mini_model.thread_count() == 9

    def test_set_thread_count_invalid(self, model):
        if IS_WINDOWS:
            with pytest.raises(OSError):
                model.set_thread_count(12)  # needs to be loaded

    def test_get_thread_count_valid(self, loaded_orca_mini_model):
        assert 0 <= loaded_orca_mini_model.thread_count() <= 4  # default; hardware dependent, but at most 4

    def test_get_thread_count_invalid(self, model):
        if IS_WINDOWS:
            with pytest.raises(OSError):
                model.thread_count()  # needs to be loaded

    def test_set_context(self, model):
        assert model.context is None  # starts without a context
        model._set_context()
        assert model.context is not None
        assert isinstance(model.context, pyllmodel.LLModelPromptContext)
        assert model.context.n_past == 0
        assert model.context.n_predict == 4096
        assert model.context.top_k == 40
        assert model.context.top_p == pytest.approx(0.9)
        assert model.context.temp == pytest.approx(0.1)
        assert model.context.n_batch == 8
        assert model.context.repeat_penalty == pytest.approx(1.2)
        assert model.context.repeat_last_n == 10
        assert model.context.context_erase == pytest.approx(0.75)
        # simulating a reset:
        model.context.n_past = 100  # pretend it has moved 100 tokens in
        model._set_context(reset_context=True)
        assert model.context is not None
        assert isinstance(model.context, pyllmodel.LLModelPromptContext)
        assert model.context.n_past == 0

    def test_generate_embedding_valid(self, loaded_embeddings_model):
        embedding = loaded_embeddings_model.generate_embedding("some text")
        assert embedding is not None
        assert len(embedding) == 384
        assert all(isinstance(e, float) for e in embedding)

    def test_generate_embedding_invalid(self, model):
        with pytest.raises(ValueError):
            model.generate_embedding(None)  # mustn't be None
        with pytest.raises(ValueError):
            model.generate_embedding("")  # mustn't be empty
        with pytest.raises(RuntimeError):
            model.generate_embedding("some text")  # model not loaded

    @pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
    def test_prompt_model(self, loaded_orca_mini_model):
        response_tokens = []

        def callback(token_id: int, response: str) -> bool:
            response_tokens.append(response)
            return True

        loaded_orca_mini_model.prompt_model("who jumped over the lazy dogs?", callback, temp=0, n_predict=9)
        expected = "\nTheir eyes were fixed on the prize"
        response_text = ''.join(response_tokens)
        print(response_text)
        # may want to revisit the following tests later (force CPU):
        # if not IS_MACOS:
        #     assert response_text == expected
        # else:
        #     assert response_text == expected or 'And let him' in response_text

    @pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
    def test_prompt_model_streaming(self, loaded_orca_mini_model):
        def callback(token_id: int, response: str) -> bool:
            return True

        generation_iter = loaded_orca_mini_model.prompt_model_streaming(
            "who jumped over the lazy dogs?", callback, temp=0, n_predict=9
        )
        expected = "\nTheir eyes were fixed on the prize"
        response_text = ''.join(generation_iter)
        print(response_text)
        # may want to revisit the following tests later (force CPU):
        # if not IS_MACOS:
        #     assert response_text == expected
        # else:
        #     assert response_text == expected or 'And let him' in response_text
