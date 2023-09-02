import os
import sys
from io import StringIO
from pathlib import Path
from typing import Iterable

from gpt4all import GPT4All, Embed4All, pyllmodel
from gpt4all.gpt4all import empty_chat_session, append_bin_suffix_if_missing
from . import get_model_folder
import time
import pytest


# running on Metal seems to cause some inference responses to differ:
IS_MACOS: bool = sys.platform == 'darwin'


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
def test_inference():
    name = 'orca-mini-3b.ggmlv3.q4_0.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    output_1 = model.generate('hello', top_k=1)

    with model.chat_session():
        response = model.generate(prompt='hello', top_k=1)
        response = model.generate(prompt='write me a short poem', top_k=1)
        response = model.generate(prompt='thank you', top_k=1)
        print(model.current_chat_session)

    output_2 = model.generate('hello', top_k=1)

    assert output_1 == output_2

    tokens = []
    for token in model.generate('hello', top_k=1, streaming=True):
        tokens.append(token)

    assert len(tokens) > 0

    with model.chat_session():
        tokens = list(model.generate(prompt='hello', top_k=1, streaming=True))
        model.current_chat_session.append({'role': 'assistant', 'content': ''.join(tokens)})

        tokens = list(model.generate(prompt='write me a poem about dogs', top_k=1, streaming=True))
        model.current_chat_session.append({'role': 'assistant', 'content': ''.join(tokens)})

        print(model.current_chat_session)


def do_long_input(model):
    long_input = " ".join(["hello how are you"] * 40)

    with model.chat_session():
        # llmodel should limit us to 128 even if we ask for more
        model.generate(long_input, n_batch=512)
        print(model.current_chat_session)


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
def test_inference_long_orca_3b():
    name = 'orca-mini-3b.ggmlv3.q4_0.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    do_long_input(model)
    assert True  # inference completed successfully


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='7B', arch='falcon')
def test_inference_long_falcon():
    name = 'ggml-model-gpt4all-falcon-q4_0.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    do_long_input(model)
    assert True  # inference completed successfully


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='7B', arch='llama_1', release='orca_mini')
def test_inference_long_llama_7b():
    name = 'orca-mini-7b.ggmlv3.q4_0.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    do_long_input(model)
    assert True  # inference completed successfully


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='13B', arch='llama_1', release='nous_hermes')
def test_inference_long_llama_13b():
    name = 'nous-hermes-13b.ggmlv3.q4_0.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    do_long_input(model)
    assert True  # inference completed successfully


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='7B', arch='mpt', release='mpt_chat')
def test_inference_long_mpt():
    name = 'ggml-mpt-7b-chat.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    do_long_input(model)
    assert True  # inference completed successfully


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='3B', arch='replit', release='replit_code_v1')
def test_inference_long_replit():
    name = 'ggml-replit-code-v1-3b.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    do_long_input(model)
    assert True  # inference completed successfully


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='7B', arch='gptj', release='groovy')
def test_inference_long_groovy():
    name = 'ggml-gpt4all-j-v1.3-groovy.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    do_long_input(model)
    assert True  # inference completed successfully


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
def test_inference_hparams():
    name = 'orca-mini-3b.ggmlv3.q4_0.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)

    output = model.generate("The capital of france is ", temp=0, max_tokens=3)
    assert 'Paris' in output or isinstance(output, str)  # 'Paris' isn't given as a response on all systems
    # assert len(output) > 0  # may want to revisit this check later


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='7B', arch='falcon')
def test_inference_falcon():
    name = 'ggml-model-gpt4all-falcon-q4_0.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    prompt = 'hello'
    output = model.generate(prompt, temp=0)
    assert isinstance(output, str)
    # assert len(output) > 0  # may want to revisit this check later


@pytest.mark.slow
@pytest.mark.online
@pytest.mark.inference(params='7B', arch='mpt', release='mpt_chat')
def test_inference_mpt():
    name = 'ggml-mpt-7b-chat.bin'
    folder = str(get_model_folder(name))
    model = GPT4All(model_name=name, model_path=folder)
    prompt = 'hello'
    output = model.generate(prompt, temp=0)
    assert isinstance(output, str)
    # assert len(output) > 0  # may want to revisit this check later


@pytest.mark.online
@pytest.mark.inference(params='22M', arch='minilm', release='minilm_l6_v2')
def test_embedding():
    text = 'The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox'
    embedder = Embed4All()
    output = embedder.embed(text)
    #for i, value in enumerate(output):
        #print(f'Value at index {i}: {value}')
    assert len(output) == 384


@pytest.mark.online
def test_empty_embedding():
    text = ''
    embedder = Embed4All()
    with pytest.raises(ValueError):
        output = embedder.embed(text)


@pytest.mark.online
def test_download_model(tmp_path: Path):
    import gpt4all.gpt4all
    old_default_dir = gpt4all.gpt4all.DEFAULT_MODEL_DIRECTORY
    gpt4all.gpt4all.DEFAULT_MODEL_DIRECTORY = tmp_path  # temporary pytest directory to ensure a download happens
    try:
        model = GPT4All(model_name='ggml-all-MiniLM-L6-v2-f16.bin')
        model_path = tmp_path / model.config['filename']
        assert model_path.absolute() == Path(model.config['path']).absolute()
        assert model_path.stat().st_size == int(model.config['filesize'])
    finally:
        gpt4all.gpt4all.DEFAULT_MODEL_DIRECTORY = old_default_dir



class TestGPT4AllClass():
    """GPT4All tests which can be run without an instance. This includes creation."""

    @pytest.mark.online
    def test_creation(self):
        name = None
        with pytest.raises(Exception):  # mustn't be None
            GPT4All(name)
        name = ''
        with pytest.raises(ValueError):  # mustn't be empty
            GPT4All(name)
        name = ' \t '  # str.isspace()
        with pytest.raises(Exception):  # mustn't be whitespace
            GPT4All(name)
        name = 'orca-mini-3b.ggmlv3.q4_0.bin'  # a valid model name
        folder = str(get_model_folder(name))
        model = GPT4All(name, folder)
        assert model is not None


    @pytest.mark.online
    def test_list_models(self):
        models_list = GPT4All.list_models()
        assert models_list is not None
        # 'models_list' is a list of config items:
        assert isinstance(models_list, list)
        # each config is a dict, with every key and value being a string:
        assert all(isinstance(m, dict) for m in models_list)
        assert all(isinstance(k, str) for m in models_list for k in m.keys())
        assert all(isinstance(v, str) for m in models_list for v in m.values())


    @pytest.mark.online
    def test_retrieve_model_online(self):
        with pytest.raises(Exception):
            GPT4All.retrieve_model(None)
        if sys.platform == 'win32':
            with pytest.raises(Exception):
                GPT4All.retrieve_model(' \t ')  # str.isspace()
        name = 'orca-mini-3b.ggmlv3.q4_0.bin'
        folder = str(get_model_folder(name))
        config = GPT4All.retrieve_model(name, folder)
        # 'config' is a dict, with every key and value being a string:
        assert isinstance(config, dict)
        assert all(isinstance(k, str) for k in config.keys())
        assert all(isinstance(v, str) for v in config.values())
        assert 'path' in config.keys()
        assert name in config['path']
        assert 'systemPrompt' in config.keys()
        assert 'promptTemplate' in config.keys()


    def test_retrieve_model_offline(self):
        with pytest.raises(Exception):
            GPT4All.retrieve_model(None, allow_download=False)
        if sys.platform == 'win32':
            with pytest.raises(Exception):
                GPT4All.retrieve_model(' \t ', allow_download=False)  # str.isspace()
        name = 'orca-mini-3b.ggmlv3.q4_0.bin'
        folder = str(get_model_folder(name))
        config = GPT4All.retrieve_model(name, folder, allow_download=False)
        # 'config' is a dict, with every key and value being a string:
        assert isinstance(config, dict)
        assert all(isinstance(k, str) for k in config.keys())
        assert all(isinstance(v, str) for v in config.values())
        assert 'path' in config.keys()
        assert name in config['path']
        assert 'systemPrompt' in config.keys()
        assert 'promptTemplate' in config.keys()


    @pytest.mark.online
    def test_download_model(self, tmp_path: Path):
        with pytest.raises(TypeError):
            GPT4All.download_model(None, None)
        with pytest.raises(Exception):
            GPT4All.download_model('', '')
        with pytest.raises(Exception):
            GPT4All.download_model(' \t ', ' \n  ')  # str.isspace()
        name = 'ggml-all-MiniLM-L6-v2-f16.bin'
        result = GPT4All.download_model(name, tmp_path)
        assert isinstance(result, str)
        assert name in result
        model_file = Path(result)
        assert model_file.exists()
        assert os.access(model_file, os.R_OK)  # model_file is readable


class TestGPT4AllMockInstance():

    @pytest.fixture
    def mock_model(self, monkeypatch) -> GPT4All:

        class MockLLModel:  # pretend to be an LLModel with access to the backend
            def __init__(self, *args, **kwargs): pass
            def load_model(self, model_path): return True
            def prompt_model(self, *args, **kwargs):
                response_callback = kwargs['callback']
                response_callback(7, 'eggs')
                response_callback(6, ' and')
                response_callback(42, ' spam.')
            def prompt_model_streaming(self, *args, **kwargs): return ['eggs', ' and', ' spam.']

        def mock_retrieve_model(*args, **kwargs):  # just the essentials, don't want file access here
            return {'path': 'mock-model.bin', 'systemPrompt': '', 'promptTemplate': '{0}'}

        monkeypatch.setattr(pyllmodel, 'LLModel', MockLLModel)
        monkeypatch.setattr(GPT4All, 'retrieve_model', mock_retrieve_model)
        name = 'mock-model.bin'
        model = GPT4All(model_name=name, allow_download=False)
        return model


    def test_generate(self, mock_model):
        prompt = "my 2 favorite fruits are"
        expected = "eggs and spam."
        response = mock_model.generate(prompt, temp=0)
        assert isinstance(response, str)
        assert response == expected


    def test_generate_with_session(self, mock_model):
        prompt1 = "name a color."
        expected1 = "eggs and spam."
        prompt2 = "now name a different color."
        expected2 = "eggs and spam."
        with mock_model.chat_session():
            response1 = mock_model.generate(prompt1, temp=0)
            assert isinstance(response1, str)
            response2 = mock_model.generate(prompt2, temp=0)
            assert isinstance(response2, str)
        assert response1 == expected1
        assert response2 == expected2


    def test_generate_streaming(self, mock_model):
        prompt = "my 2 favorite fruits are"
        expected = "eggs and spam."
        response = mock_model.generate(prompt, streaming=True, temp=0)
        assert isinstance(response, Iterable)
        response = list(response)
        assert ''.join(response) == expected


    def test_generate_streaming_with_session(self, mock_model):
        prompt1 = "name a color."
        expected1 = "eggs and spam."
        prompt2 = "now name a different color."
        expected2 = "eggs and spam."
        with mock_model.chat_session():
            response1 = mock_model.generate(prompt1, streaming=True, temp=0)
            assert isinstance(response1, Iterable)
            response1 = list(response1)
            response2 = mock_model.generate(prompt2, streaming=True, temp=0)
            assert isinstance(response2, Iterable)
            response2 = list(response2)
        assert ''.join(response1) == expected1
        assert ''.join(response2) == expected2


    def test_format_chat_prompt_template(self, mock_model):
        messages = [{
            'role': 'user',
            'content': 'User: hello\n',
        }, {
            'role': 'assistant',
            'content': 'Assistant: hello to you, too, good sir!'
        }]
        expected = "User: hello\nAssistant: hello to you, too, good sir!\n"
        formatted_chat_prompt_template = mock_model._format_chat_prompt_template(messages)
        assert isinstance(formatted_chat_prompt_template, str)
        assert formatted_chat_prompt_template == expected



class TestGPT4AllInstance():

    @pytest.fixture
    def model(self) -> GPT4All:
        name = 'orca-mini-3b.ggmlv3.q4_0.bin'
        folder = str(get_model_folder(name))
        model = GPT4All(model_name=name, model_path=folder)
        yield model
        del model.model  # making sure the memory gets freed asap
        del model


    @pytest.mark.slow
    @pytest.mark.online
    @pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
    def test_generate(self, model):
        prompt = "my 2 favorite fruits are"
        expected = " apples and bananas.\nI love to eat apples and bananas."
        response = model.generate(prompt, temp=0)
        assert isinstance(response, str)
        # may want to revisit the following tests later (force CPU):
        # if not IS_MACOS:  # running on Metal can result in a different output
        #     assert response == expected
        # else:
        #     assert response == expected or 'peaches' in response


    @pytest.mark.slow
    @pytest.mark.online
    @pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
    def test_generate_with_session(self, model):
        prompt1 = "name a color."
        expected1 = " Blue."
        prompt2 = "now name a different color."
        expected2 = " Green."
        with model.chat_session():
            response1 = model.generate(prompt1, temp=0)
            assert isinstance(response1, str)
            response2 = model.generate(prompt2, temp=0)
            assert isinstance(response2, str)
        # may want to revisit the following tests later (force CPU):
        # assert response1 == expected1
        # assert response2 == expected2


    @pytest.mark.slow
    @pytest.mark.online
    @pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
    def test_generate_streaming(self, model):
        prompt = "my 2 favorite fruits are"
        expected = " apples and bananas.\nI love to eat apples and bananas."
        response = model.generate(prompt, streaming=True, temp=0)
        assert isinstance(response, Iterable)
        response = list(response)
        print(response)
        response_text = ''.join(response)
        # may want to revisit the following tests later (force CPU):
        # if not IS_MACOS:  # running on Metal can result in a different output
        #     assert response_text == expected
        # else:
        #     assert response_text == expected or 'peaches' in response_text


    @pytest.mark.slow
    @pytest.mark.online
    @pytest.mark.inference(params='3B', arch='llama_1', release='orca_mini')
    def test_generate_streaming_with_session(self, model):
        prompt1 = "name a color."
        expected1 = " Blue."
        prompt2 = "now name a different color."
        expected2 = " Green."
        with model.chat_session():
            response1 = model.generate(prompt1, streaming=True, temp=0)
            assert isinstance(response1, Iterable)
            response1 = list(response1)
            response2 = model.generate(prompt2, streaming=True, temp=0)
            assert isinstance(response2, Iterable)
            response2 = list(response2)
        print(response1)
        print(response2)
        # may want to revisit the following tests later (force CPU):
        # assert ''.join(response1) == expected1
        # assert ''.join(response2) == expected2



@pytest.mark.parametrize('system_prompt', ['', 'System: Hi.', '### System: Be helpful, damnit.'])
def test_empty_chat_session(system_prompt):
    chat_session = empty_chat_session(system_prompt)
    assert isinstance(chat_session, list)
    len(chat_session) == 1
    assert all(k in ('role', 'content') for k in chat_session[0].keys())
    assert all(isinstance(v, str) for v in chat_session[0].values())
    assert chat_session[0]['content'] == system_prompt


@pytest.mark.parametrize('model_name', [None, '', 'mock-model.bin', 'orca-mini-3b.ggmlv3.q4_0'])
def test_append_bin_suffix(model_name):
    if not hasattr(model_name, 'endswith'):  # duck typing test
        with pytest.raises(Exception):
            append_bin_suffix_if_missing(model_name)
    else:
        new_model_name = append_bin_suffix_if_missing(model_name)
        assert new_model_name[-4:] == '.bin'
