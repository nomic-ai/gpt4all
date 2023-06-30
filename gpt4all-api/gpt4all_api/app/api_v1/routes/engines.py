import logging
from typing import Dict, List

from api_v1.settings import settings
from fastapi import APIRouter, Depends, Response, Security, status
from pydantic import BaseModel, Field

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

### This should follow https://github.com/openai/openai-openapi/blob/master/openapi.yaml


class ListEnginesResponse(BaseModel):
    data: List[Dict] = Field(..., description="All available models.")


class EngineResponse(BaseModel):
    data: List[Dict] = Field(..., description="All available models.")


router = APIRouter(prefix="/engines", tags=["Search Endpoints"])


@router.get("/", response_model=ListEnginesResponse)
async def list_engines():
    '''
    List all available GPT4All models from
    https://raw.githubusercontent.com/nomic-ai/gpt4all/main/gpt4all-chat/metadata/models.json
    '''
    raise NotImplementedError()
    return ListEnginesResponse(data=[])


@router.get("/{engine_id}", response_model=EngineResponse)
async def retrieve_engine(engine_id: str):
    ''' '''

    raise NotImplementedError()
    return EngineResponse()
