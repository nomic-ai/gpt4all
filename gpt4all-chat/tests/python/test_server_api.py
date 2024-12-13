import os
import shutil
import signal
import subprocess
import sys
import tempfile
import textwrap
from contextlib import contextmanager
from pathlib import Path
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

    def get(self, path: str, *, raise_for_status: bool = True, wait: bool = False) -> Any:
        return self._request('GET', path, raise_for_status=raise_for_status, wait=wait)

    def post(self, path: str, data: dict[str, Any] | None, *, raise_for_status: bool = True, wait: bool = False) -> Any:
        return self._request('POST', path, data, raise_for_status=raise_for_status, wait=wait)

    def _request(
        self, method: str, path: str, data: dict[str, Any] | None = None, *, raise_for_status: bool, wait: bool,
    ) -> Any:
        if wait:
            retry = Retry(total=None, connect=10, read=False, status=0, other=0, backoff_factor=.01)
        else:
            retry = Retry(total=False)
        self.http_adapter.max_retries = retry  # type: ignore[attr-defined]

        resp = self.session.request(method, f'http://localhost:4891/v1/{path}', json=data)
        if raise_for_status:
            resp.raise_for_status()
            return resp.json()

        try:
            json_data = resp.json()
        except ValueError:
            json_data = None
        return resp.status_code, json_data


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
        local_env_file_path = Path(os.environ['TEST_MODEL_PATH'])
        shutil.copy(local_env_file_path, app_data_dir / local_env_file_path.name)

    return dict(
        os.environ,
        XDG_CACHE_HOME=str(tmpdir / 'cache'),
        XDG_DATA_HOME=str(tmpdir / 'share'),
        XDG_CONFIG_HOME=str(xdg_confdir),
        APPIMAGE=str(tmpdir),  # hack to bypass SingleApplication
    )


@contextmanager
def prepare_chat_server(model_copied: bool = False) -> Iterator[dict[str, str]]:
    if os.name != 'posix' or sys.platform == 'darwin':
        pytest.skip('Need non-Apple Unix to use alternate config path')

    with tempfile.TemporaryDirectory(prefix='gpt4all-test') as td:
        tmpdir = Path(td)
        config = create_chat_server_config(tmpdir, model_copied=model_copied)
        yield config


def start_chat_server(config: dict[str, str]) -> Iterator[None]:
    chat_executable = Path(os.environ['CHAT_EXECUTABLE']).absolute()
    with subprocess.Popen(chat_executable, env=config) as process:
        try:
            yield
        except:
            process.kill()
            raise
        process.send_signal(signal.SIGINT)
        if retcode := process.wait():
            raise CalledProcessError(retcode, process.args)


@pytest.fixture
def chat_server() -> Iterator[None]:
    with prepare_chat_server(model_copied=False) as config:
        yield from start_chat_server(config)


@pytest.fixture
def chat_server_with_model() -> Iterator[None]:
    with prepare_chat_server(model_copied=True) as config:
        yield from start_chat_server(config)


def test_with_models_empty(chat_server: None) -> None:
    # non-sense endpoint
    status_code, response = request.get('foobarbaz', wait=True, raise_for_status=False)
    assert status_code == 404
    assert response is None

    # empty model list
    response = request.get('models')
    assert response == {'object': 'list', 'data': []}

    # empty model info
    response = request.get('models/foo')
    assert response == {}

    # POST for model list
    status_code, response = request.post('models', data=None, raise_for_status=False)
    assert status_code == 405
    assert response == {'error': {
        'code': None,
        'message': 'Not allowed to POST on /v1/models. (HINT: Perhaps you meant to use a different HTTP method?)',
        'param': None,
        'type': 'invalid_request_error',
    }}

    # POST for model info
    status_code, response = request.post('models/foo', data=None, raise_for_status=False)
    assert status_code == 405
    assert response == {'error': {
        'code': None,
        'message': 'Not allowed to POST on /v1/models/*. (HINT: Perhaps you meant to use a different HTTP method?)',
        'param': None,
        'type': 'invalid_request_error',
    }}

    # GET for completions
    status_code, response = request.get('completions', raise_for_status=False)
    assert status_code == 405
    assert response == {'error': {
        'code': 'method_not_supported',
        'message': 'Only POST requests are accepted.',
        'param': None,
        'type': 'invalid_request_error',
    }}

    # GET for chat completions
    status_code, response = request.get('chat/completions', raise_for_status=False)
    assert status_code == 405
    assert response == {'error': {
        'code': 'method_not_supported',
        'message': 'Only POST requests are accepted.',
        'param': None,
        'type': 'invalid_request_error',
    }}


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
            'finish_reason': 'length',
            'index': 0,
            'logprobs': None,
            'text': ' jumps over the lazy dog.\n',
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
    response = request.get('models/Llama 3.2 1B Instruct')
    assert response == EXPECTED_MODEL_INFO

    # Test the completions endpoint
    status_code, response = request.post('completions', data=None, raise_for_status=False)
    assert status_code == 400
    assert response == {'error': {
        'code': None,
        'message': 'error parsing request JSON: illegal value',
        'param': None,
        'type': 'invalid_request_error',
    }}

    data = dict(
        model       = 'Llama 3.2 1B Instruct',
        prompt      = 'The quick brown fox',
        temperature = 0,
        max_tokens  = 6,
    )
    response = request.post('completions', data=data)
    del response['created']  # Remove the dynamic field for comparison
    assert response == EXPECTED_COMPLETIONS_RESPONSE


def test_with_models_temperature(chat_server_with_model: None) -> None:
    """Fixed by nomic-ai/gpt4all#3202."""
    data = {
        'model': 'Llama 3.2 1B Instruct',
        'prompt': 'The quick brown fox',
        'temperature': 0.5,
    }

    request.post('completions', data=data, wait=True, raise_for_status=True)
