import logging
import time
from typing import Dict, List

from api_v1.settings import settings
from fastapi import APIRouter, Depends, Response, Security, status
from pydantic import BaseModel, Field

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

### This should follow https://github.com/openai/openai-openapi/blob/master/openapi.yaml


class ChatCompletionMessage(BaseModel):
    role: str
    content: str


class ChatCompletionRequest(BaseModel):
    model: str = Field(..., description='The model to generate a completion from.')
    messages: List[ChatCompletionMessage] = Field(..., description='The model to generate a completion from.')


class ChatCompletionChoice(BaseModel):
    message: ChatCompletionMessage
    index: int
    finish_reason: str


class ChatCompletionUsage(BaseModel):
    prompt_tokens: int
    completion_tokens: int
    total_tokens: int


class ChatCompletionResponse(BaseModel):
    id: str
    object: str = 'text_completion'
    created: int
    model: str
    choices: List[ChatCompletionChoice]
    usage: ChatCompletionUsage


router = APIRouter(prefix="/chat", tags=["Completions Endpoints"])


@router.post("/completions", response_model=ChatCompletionResponse)
async def chat_completion(request: ChatCompletionRequest):
    '''
    Completes a GPT4All model response.
    '''

    return ChatCompletionResponse(
        id='asdf',
        created=time.time(),
        model=request.model,
        choices=[{}],
        usage={'prompt_tokens': 0, 'completion_tokens': 0, 'total_tokens': 0},
    )
