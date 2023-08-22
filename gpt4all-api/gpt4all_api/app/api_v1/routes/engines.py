import json
import logging
from typing import Dict, List
import aiohttp

from requests import request

from api_v1.settings import settings
from fastapi import APIRouter, Depends, Response, Security, status
from pydantic import BaseModel, Field, ValidationError
from api_v1.models.engine_model import Engine, EngineResponse, ListEnginesResponse

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

# This should follow https://github.com/openai/openai-openapi/blob/master/openapi.yaml


router = APIRouter(prefix="/engines", tags=["Search Endpoints"])


@router.get("/", response_model=ListEnginesResponse)
async def list_engines() -> ListEnginesResponse:
    '''
    List all available GPT4All models from
    https://raw.githubusercontent.com/nomic-ai/gpt4all/main/gpt4all-chat/metadata/models.json
    '''

    url: str = "https://raw.githubusercontent.com/nomic-ai/gpt4all/main/gpt4all-chat/metadata/models.json"

    async with aiohttp.ClientSession() as session:
        try:
            logger.debug("getting data from url: {}".format(url))
            async with session.get(url) as response:
                json_str: str = await response.text()
                logger.debug("request status: {}".format(response.status))
                engines: List[Engine] = [Engine(**e)
                                         for e in json.loads(json_str)]
                return ListEnginesResponse(data=engines)

        except aiohttp.ClientError as e:
            logger.error(f"Client error: {e}")
        except json.JSONDecodeError as e:
            logger.error(f"JSON decoding error: {e}")
        except ValidationError as e:
            logger.error(f"JSON validation error: {e}")
        except Exception as e:
            logger.error(f"Unexpected error: {e}")

    return ListEnginesResponse(data=[])


@router.get("/{engine_id}", response_model=EngineResponse)
async def retrieve_engine(engine_id: str):
    ''' '''

    raise NotImplementedError()
    return EngineResponse()
