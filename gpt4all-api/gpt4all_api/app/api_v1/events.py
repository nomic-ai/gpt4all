import logging

from api_v1.settings import settings
from fastapi import HTTPException
from fastapi.responses import JSONResponse
from starlette.requests import Request

log = logging.getLogger(__name__)


startup_msg_fmt = """
 Starting up GPT4All API
"""


async def on_http_error(request: Request, exc: HTTPException):
    return JSONResponse({'detail': exc.detail}, status_code=exc.status_code)


async def on_startup(app):
    startup_msg = startup_msg_fmt.format(settings=settings)
    log.info(startup_msg)


def startup_event_handler(app):
    async def start_app() -> None:
        await on_startup(app)

    return start_app
