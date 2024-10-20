import os
import signal
import subprocess
import sys
import tempfile
import textwrap
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

    def get(self, path: str, *, wait: bool = False) -> Any:
        return self._request('GET', path, wait)

    def _request(self, method: str, path: str, wait: bool) -> Any:
        if wait:
            retry = Retry(total=None, connect=10, read=False, status=0, other=0, backoff_factor=.01)
        else:
            retry = Retry(total=False)
        self.http_adapter.max_retries = retry  # type: ignore[attr-defined]

        resp = self.session.request(method, f'http://localhost:4891/v1/{path}')
        resp.raise_for_status()
        return resp.json()


request = Requestor()


@pytest.fixture
def chat_server_config() -> Iterator[dict[str, str]]:
    if os.name != 'posix' or sys.platform == 'darwin':
        pytest.skip('Need non-Apple Unix to use alternate config path')

    with tempfile.TemporaryDirectory(prefix='gpt4all-test') as td:
        tmpdir = Path(td)
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
        yield dict(
            os.environ,
            XDG_CACHE_HOME=str(tmpdir / 'cache'),
            XDG_DATA_HOME=str(tmpdir / 'share'),
            XDG_CONFIG_HOME=str(xdg_confdir),
            APPIMAGE=str(tmpdir),  # hack to bypass SingleApplication
        )


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


def test_list_models_empty(chat_server: None) -> None:
    assert request.get('models', wait=True) == {'object': 'list', 'data': []}
