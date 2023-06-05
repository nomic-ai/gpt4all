import os
import docs
import logging
from fastapi import FastAPI, HTTPException, Request
from starlette.middleware.cors import CORSMiddleware
from fastapi.logger import logger as fastapi_logger
from api_v1.settings import settings
from api_v1.api import router as v1_router
from api_v1 import events

logger = logging.getLogger(__name__)

app = FastAPI(title='GPT4All API', description=docs.desc)


origins = [f"http://{settings.client_hostname}/", f"https://{settings.client_hostname}/"]

#CORS Configuration (in-case you want to deploy)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"] + origins,
    allow_credentials=True,
    allow_methods=["GET", "POST", "OPTIONS"],
    allow_headers=["*"],
)

logger.info('Adding v1 endpoints..')

# add v1
app.include_router(v1_router, prefix='/v1')
app.add_event_handler('startup', events.startup_event_handler(app))
app.add_exception_handler(HTTPException, events.on_http_error)


@app.on_event("startup")
async def startup():
    logger.info("GPT4All API is ready.")


@app.on_event("shutdown")
async def shutdown():
    logger.info("Shutting down API")


# This is needed to get logs to show up in the app
if "gunicorn" in os.environ.get("SERVER_SOFTWARE", ""):
    gunicorn_error_logger = logging.getLogger("gunicorn.error")
    gunicorn_logger = logging.getLogger("gunicorn")

    root_logger = logging.getLogger()
    fastapi_logger.setLevel(gunicorn_logger.level)
    fastapi_logger.handlers = gunicorn_error_logger.handlers
    root_logger.setLevel(gunicorn_logger.level)

    uvicorn_logger = logging.getLogger("uvicorn.access")
    uvicorn_logger.handlers = gunicorn_error_logger.handlers
else:
    # https://github.com/tiangolo/fastapi/issues/2019
    LOG_FORMAT2 = "[%(asctime)s %(process)d:%(threadName)s] %(name)s - %(levelname)s - %(message)s | %(filename)s:%(lineno)d"
    logging.basicConfig(level=logging.INFO, format=LOG_FORMAT2)