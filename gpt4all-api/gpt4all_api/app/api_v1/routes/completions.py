from fastapi import APIRouter, Depends, Response, Security, status
from pydantic import BaseModel, Field
from typing import List, Dict
import logging
from api_v1.settings import settings
import time

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

### This should follow https://github.com/openai/openai-openapi/blob/master/openapi.yaml

class CompletionRequest(BaseModel):
    model: str = Field(..., description='The model to generate a completion from.')
    prompt: str = Field(..., description='The prompt to begin completing from.')
    max_tokens: int = Field(7, description='Max tokens to generate')
    temperature: float = Field(0, description='Model temperature')
    top_p: float = Field(1.0, description='top_p')
    n: int = Field(1, description='')
    stream: bool = Field(False, description='Stream responses')


class CompletionChoice(BaseModel):
    text: str
    index: int
    logprobs: float
    finish_reason: str

class CompletionUsage(BaseModel):
    prompt_tokens: int
    completion_tokens: int
    total_tokens: int
class CompletionResponse(BaseModel):
    id: str
    object: str = 'text_completion'
    created: int
    model: str
    choices: List[CompletionChoice]
    usage: CompletionUsage


router = APIRouter(prefix="/completions", tags=["Completion Endpoints"])

@router.post("/", response_model=CompletionResponse)
async def completions(request: CompletionRequest):
    '''
    Completes a GPT4All model response.
    '''


    return CompletionResponse(
        id='asdf',
        created=time.time(),
        model=request.model,
        choices=[],
        usage={
            'prompt_tokens': 0,
            'completion_tokens': 0,
            'total_tokens': 0
        }
    )


