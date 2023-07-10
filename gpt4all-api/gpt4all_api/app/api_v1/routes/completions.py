import json

from fastapi import APIRouter, Depends, Response, Security, status
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field
from typing import List, Dict, Iterable, AsyncIterable
import logging
from uuid import uuid4
from api_v1.settings import settings
from gpt4all import GPT4All
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


class CompletionStreamResponse(BaseModel):
    id: str
    object: str = 'text_completion'
    created: int
    model: str
    choices: List[CompletionChoice]


router = APIRouter(prefix="/completions", tags=["Completion Endpoints"])


def stream_completion(output: Iterable, base_response: CompletionStreamResponse):
    """
    Streams a GPT4All output to the client.

    Args:
        output: The output of GPT4All.generate(), which is an iterable of tokens.
        base_response: The base response object, which is cloned and modified for each token.

    Returns:
        A Generator of CompletionStreamResponse objects, which are serialized to JSON Event Stream format.
    """
    for token in output:
        chunk = base_response.copy()
        chunk.choices = [dict(CompletionChoice(
            text=token,
            index=0,
            logprobs=-1,
            finish_reason=''
        ))]
        yield f"data: {json.dumps(dict(chunk))}\n\n"


@router.post("/", response_model=CompletionResponse)
async def completions(request: CompletionRequest):
    '''
    Completes a GPT4All model response.
    '''

    model = GPT4All(model_name=settings.model, model_path=settings.gpt4all_path)

    output = model.generate(prompt=request.prompt,
                            n_predict=request.max_tokens,
                            streaming=request.stream,
                            top_k=20,
                            top_p=request.top_p,
                            temp=request.temperature,
                            n_batch=1024,
                            repeat_penalty=1.2,
                            repeat_last_n=10)

    # If streaming, we need to return a StreamingResponse
    if request.stream:
        base_chunk = CompletionStreamResponse(
            id=str(uuid4()),
            created=time.time(),
            model=request.model,
            choices=[]
        )
        return StreamingResponse((response for response in stream_completion(output, base_chunk)),
                                 media_type="text/event-stream")
    else:
        return CompletionResponse(
            id=str(uuid4()),
            created=time.time(),
            model=request.model,
            choices=[dict(CompletionChoice(
                text=output,
                index=0,
                logprobs=-1,
                finish_reason='stop'
            ))],
            usage={
                'prompt_tokens': 0, #TODO how to compute this?
                'completion_tokens': 0,
                'total_tokens': 0
            }
        )
