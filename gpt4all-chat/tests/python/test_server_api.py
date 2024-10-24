import os
import shutil
import signal
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path
from requests.exceptions import HTTPError
from subprocess import CalledProcessError
from typing import Any, Iterator

import pytest
import requests
from urllib3 import Retry

from . import config


class Requestor:
    def __init__(self) -> None:
        self.session = requests.Session()
        self.http_adapter = self.session.adapters['http://']

    def get(self, path: str, *, wait: bool = False) -> Any:
        return self._request('GET', path, wait)

    def post(self, path: str, data: dict[str, Any] | None, *, wait: bool = False) -> Any:
        return self._request('POST', path, wait, data)

    def _request(self, method: str, path: str, wait: bool, data: dict[str, Any] | None = None) -> Any:
        if wait:
            retry = Retry(total=None, connect=10, read=False, status=0, other=0, backoff_factor=.01)
        else:
            retry = Retry(total=False)
        self.http_adapter.max_retries = retry  # type: ignore[attr-defined]

        resp = self.session.request(method, f'http://localhost:4891/v1/{path}', json=data)
        resp.raise_for_status()
        return resp.json()


request = Requestor()


def create_chat_server_config(tmpdir: Path, model_copied: bool = False) -> dict[str, str]:
    xdg_confdir = tmpdir / 'config'
    app_confdir = xdg_confdir / 'nomic.ai'
    app_confdir.mkdir(parents=True)
    with open(app_confdir / 'GPT4All.ini', 'w') as conf:
        conf.write(textwrap.dedent(f"""\
            [General]
            serverChat=true

            [download]
            lastVersionStarted={config.APP_VERSION}

            [network]
            isActive=false
            usageStatsActive=false
        """))

    if model_copied:
        app_data_dir = tmpdir / 'share' / 'nomic.ai' / 'GPT4All'
        app_data_dir.mkdir(parents=True)
        local_file_path_env = os.getenv('TEST_MODEL_PATH')
        if local_file_path_env:
            local_file_path = Path(local_file_path_env)
            if local_file_path.exists():
                shutil.copy(local_file_path, app_data_dir / local_file_path.name)
            else:
                pytest.fail(f'Model file specified in TEST_MODEL_PATH does not exist: {local_file_path}')
        else:
            pytest.fail('Environment variable TEST_MODEL_PATH is not set')

    return dict(
        os.environ,
        XDG_CACHE_HOME=str(tmpdir / 'cache'),
        XDG_DATA_HOME=str(tmpdir / 'share'),
        XDG_CONFIG_HOME=str(xdg_confdir),
        APPIMAGE=str(tmpdir),  # hack to bypass SingleApplication
    )


@pytest.fixture
def chat_server_config() -> Iterator[dict[str, str]]:
    if os.name != 'posix' or sys.platform == 'darwin':
        pytest.skip('Need non-Apple Unix to use alternate config path')

    with tempfile.TemporaryDirectory(prefix='gpt4all-test') as td:
        tmpdir = Path(td)
        yield create_chat_server_config(tmpdir, model_copied=False)


@pytest.fixture
def chat_server_with_model_config() -> Iterator[dict[str, str]]:
    if os.name != 'posix' or sys.platform == 'darwin':
        pytest.skip('Need non-Apple Unix to use alternate config path')

    with tempfile.TemporaryDirectory(prefix='gpt4all-test') as td:
        tmpdir = Path(td)
        yield create_chat_server_config(tmpdir, model_copied=True)


@pytest.fixture
def chat_server(chat_server_config: dict[str, str]) -> Iterator[None]:
    chat_executable = Path(os.environ['CHAT_EXECUTABLE']).absolute()
    with subprocess.Popen(chat_executable, env=chat_server_config) as process:
        try:
            yield
        except:
            process.kill()
            raise
        process.send_signal(signal.SIGINT)
        if retcode := process.wait():
            raise CalledProcessError(retcode, process.args)


@pytest.fixture
def chat_server_with_model(chat_server_with_model_config: dict[str, str]) -> Iterator[None]:
    chat_executable = Path(os.environ['CHAT_EXECUTABLE']).absolute()
    with subprocess.Popen(chat_executable, env=chat_server_with_model_config) as process:
        try:
            yield
        except:
            process.kill()
            raise
        process.send_signal(signal.SIGINT)
        if retcode := process.wait():
            raise CalledProcessError(retcode, process.args)


def test_with_models_empty(chat_server: None) -> None:
    # non-sense endpoint
    with pytest.raises(HTTPError) as excinfo:
        request.get('foobarbaz', wait=True)
    assert excinfo.value.response.status_code == 404

    # empty model list
    response = request.get('models', wait=True)
    assert response == {'object': 'list', 'data': []}

    # empty model info
    response = request.get('models/foo', wait=True)
    assert response == {}

    # POST for model list
    with pytest.raises(HTTPError) as excinfo:
        response = request.post('models', data=None, wait=True)
    assert excinfo.value.response.status_code == 405

    # POST for model info
    with pytest.raises(HTTPError) as excinfo:
        response = request.post('models/foo', data=None, wait=True)
    assert excinfo.value.response.status_code == 405

    # GET for completions
    with pytest.raises(HTTPError) as excinfo:
        response = request.get('completions', wait=True)
    assert excinfo.value.response.status_code == 405

    # GET for chat completions
    with pytest.raises(HTTPError) as excinfo:
        response = request.get('chat/completions', wait=True)
    assert excinfo.value.response.status_code == 405


EXPECTED_MODEL_INFO = {
    'created': 0,
    'id': 'Llama 3.2 1B Instruct',
    'object': 'model',
    'owned_by': 'humanity',
    'parent': None,
    'permissions': [
        {
            'allow_create_engine': False,
            'allow_fine_tuning': False,
            'allow_logprobs': False,
            'allow_sampling': False,
            'allow_search_indices': False,
            'allow_view': True,
            'created': 0,
            'group': None,
            'id': 'placeholder',
            'is_blocking': False,
            'object': 'model_permission',
            'organization': '*',
        },
    ],
    'root': 'Llama 3.2 1B Instruct',
}

EXPECTED_COMPLETIONS_RESPONSE = {
    'choices': [
        {
            'finish_reason': 'stop',
            'index': 0,
            'logprobs': None,
            'references': None,
            'text': ' jumps over the lazy dog.',
        },
    ],
    'id': 'placeholder',
    'model': 'Llama 3.2 1B Instruct',
    'object': 'text_completion',
    'usage': {
        'completion_tokens': 6,
        'prompt_tokens': 5,
        'total_tokens': 11,
    },
}


def test_with_models(chat_server_with_model: None) -> None:
    response = request.get('models', wait=True)
    assert response == {
        'data': [EXPECTED_MODEL_INFO],
        'object': 'list',
    }

    # Test the specific model endpoint
    response = request.get('models/Llama 3.2 1B Instruct', wait=True)
    assert response == EXPECTED_MODEL_INFO

    # Test the completions endpoint
    with pytest.raises(HTTPError) as excinfo:
        request.post('completions', data=None, wait=True)
    assert excinfo.value.response.status_code == 400

    data = {
        'model': 'Llama 3.2 1B Instruct',
        'prompt': 'The quick brown fox',
        'temperature': 0,
    }

    response = request.post('completions', data=data, wait=True)
    assert 'choices' in response
    assert response['choices'][0]['text'] == ' jumps over the lazy dog.'
    assert 'created' in response
    response.pop('created')  # Remove the dynamic field for comparison
    assert response == EXPECTED_COMPLETIONS_RESPONSE

    data['temperature'] = 0.5

    pytest.xfail('This causes an assertion failure in the app. See https://github.com/nomic-ai/gpt4all/issues/3133')
    with pytest.raises(HTTPError) as excinfo:
        response = request.post('completions', data=data, wait=True)
