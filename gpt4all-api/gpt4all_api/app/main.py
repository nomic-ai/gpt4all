import logging
import os

import docs
from api_v1 import events
from api_v1.api import router as v1_router
from api_v1.settings import settings
from fastapi import FastAPI, HTTPException, Request
from fastapi.logger import logger as fastapi_logger
from starlette.middleware.cors import CORSMiddleware

logger = logging.getLogger(__name__)

app = FastAPI(title='GPT4All API', description=docs.desc)

# CORS Configuration (in-case you want to deploy)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
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
    global model
    if settings.inference_mode == "cpu":
        logger.info(f"Downloading/fetching model: {os.path.join(settings.gpt4all_path, settings.model)}")
        from gpt4all import GPT4All

        model = GPT4All(model_name=settings.model, model_path=settings.gpt4all_path)

        logger.info(f"GPT4All API is ready to infer from {settings.model} on CPU.")

    else:
        # is it possible to do this once the server is up?
        ## TODO block until HF inference server is up.
        logger.info(f"GPT4All API is ready to infer from {settings.model} on CPU.")



@app.on_event("shutdown")
async def shutdown():
    logger.info("Shutting down API")


if settings.sentry_dns is not None:
    import sentry_sdk

    def traces_sampler(sampling_context):
        if 'health' in sampling_context['transaction_context']['name']:
            return False

    sentry_sdk.init(
        dsn=settings.sentry_dns, traces_sample_rate=0.1, traces_sampler=traces_sampler, send_default_pii=False
    )

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
    LOG_FORMAT2 = (
        "[%(asctime)s %(process)d:%(threadName)s] %(name)s - %(levelname)s - %(message)s | %(filename)s:%(lineno)d"
    )
    logging.basicConfig(level=logging.INFO, format=LOG_FORMAT2)
