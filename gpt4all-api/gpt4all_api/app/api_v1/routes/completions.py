import json
from typing import List, Dict, Iterable, AsyncIterable
import logging
import time
from typing import Dict, List, Union, Optional
from uuid import uuid4
import aiohttp
import asyncio
from api_v1.settings import settings
from fastapi import APIRouter, Depends, Response, Security, status, HTTPException
from fastapi.responses import StreamingResponse
from gpt4all import GPT4All
from pydantic import BaseModel, Field

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


### This should follow https://github.com/openai/openai-openapi/blob/master/openapi.yaml


class CompletionRequest(BaseModel):
    model: str = Field(settings.model, description='The model to generate a completion from.')
    prompt: Union[List[str], str] = Field(..., description='The prompt to begin completing from.')
    max_tokens: int = Field(None, description='Max tokens to generate')
    temperature: float = Field(settings.temp, description='Model temperature')
    top_p: Optional[float] = Field(settings.top_p, description='top_p')
    top_k: Optional[int] = Field(settings.top_k, description='top_k')
    n: int = Field(1, description='How many completions to generate for each prompt')
    stream: bool = Field(False, description='Stream responses')
    repeat_penalty: float = Field(settings.repeat_penalty, description='Repeat penalty')


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

async def gpu_infer(payload, header):
    async with aiohttp.ClientSession() as session:
        try:
            async with session.post(
                settings.hf_inference_server_host, headers=header, data=json.dumps(payload)
            ) as response:
                resp = await response.json()
            return resp

        except aiohttp.ClientError as e:
            # Handle client-side errors (e.g., connection error, invalid URL)
            logger.error(f"Client error: {e}")
        except aiohttp.ServerError as e:
            # Handle server-side errors (e.g., internal server error)
            logger.error(f"Server error: {e}")
        except json.JSONDecodeError as e:
            # Handle JSON decoding errors
            logger.error(f"JSON decoding error: {e}")
        except Exception as e:
            # Handle other unexpected exceptions
            logger.error(f"Unexpected error: {e}")

@router.post("/", response_model=CompletionResponse)
async def completions(request: CompletionRequest):
    '''
    Completes a GPT4All model response.
    '''
    if settings.inference_mode == "gpu":
        params = request.dict(exclude={'model', 'prompt', 'max_tokens', 'n'})
        params["max_new_tokens"] = request.max_tokens
        params["num_return_sequences"] = request.n

        header = {"Content-Type": "application/json"}
        if isinstance(request.prompt, list):
            tasks = []
            for prompt in request.prompt:
                payload = {"parameters": params}
                payload["inputs"] = prompt
                task = gpu_infer(payload, header)
                tasks.append(task)
            results = await asyncio.gather(*tasks)

            choices = []
            for response in results:
                scores = response["scores"] if "scores" in response else -1.0
                choices.append(
                    dict(
                        CompletionChoice(
                            text=response["generated_text"], index=0, logprobs=scores, finish_reason='stop'
                        )
                    )
                )

            return CompletionResponse(
                id=str(uuid4()),
                created=time.time(),
                model=request.model,
                choices=choices,
                usage={'prompt_tokens': 0, 'completion_tokens': 0, 'total_tokens': 0},
            )

        else:
            payload = {"parameters": params}
            # If streaming, we need to return a StreamingResponse
            payload["inputs"] = request.prompt

            resp = await gpu_infer(payload, header)

            output = resp["generated_text"]
            # this returns all logprobs
            scores = resp["scores"] if "scores" in resp else -1.0

            return CompletionResponse(
                id=str(uuid4()),
                created=time.time(),
                model=request.model,
                choices=[dict(CompletionChoice(text=output, index=0, logprobs=scores, finish_reason='stop'))],
                usage={'prompt_tokens': 0, 'completion_tokens': 0, 'total_tokens': 0},
            )

    else:

        if request.model != settings.model:
            raise HTTPException(status_code=400,
                                detail=f"The GPT4All inference server is booted to only infer: `{settings.model}`")

        if isinstance(request.prompt, list):
            if len(request.prompt) > 1:
                raise HTTPException(status_code=400, detail="Can only infer one inference per request in CPU mode.")
            else:
                request.prompt = request.prompt[0]

        model = GPT4All(model_name=settings.model, model_path=settings.gpt4all_path)

        output = model.generate(prompt=request.prompt,
                                max_tokens=request.max_tokens,
                                streaming=request.stream,
                                top_k=request.top_k,
                                top_p=request.top_p,
                                temp=request.temperature,
                                )

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
                    'prompt_tokens': 0,  # TODO how to compute this?
                    'completion_tokens': 0,
                    'total_tokens': 0
                }
            )
