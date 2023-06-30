import logging
import time
from typing import Dict, List
from uuid import uuid4

import aiohttp
from api_v1.settings import settings
from fastapi import APIRouter, Depends, Response, Security, status
from pydantic import BaseModel, Field

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


URL = "http://localhost:8000/generate"


# mimicing huggingface `.generate` functionality
# kwargs can be found here: https://huggingface.co/docs/transformers/main_classes/text_generation#transformers.GenerationConfig
class GenerationRequest(BaseModel):
    model: str = Field(..., description='The model being served.')
    prompt: str = Field(..., description='The prompt to begin generate from.')
    url: str = Field(..., description='The url of the locally hosted model.')
    max_new_tokens: int = Field(10, description='Max tokens to generate')
    temperature: float = Field(1.0, description='Model temperature')
    top_p: float = Field(1.0, description='top_p')
    top_k: int = Field(50, description='top_k')
    output_scores: bool = Field(False, description='Whether to output scores')
    repeat_penalty: float = Field(1.0, description='Repeat penalty')
    do_sample: bool = Field(True, description='Whether to sample')
    num_beams: int = Field(1, description='Number of beams')


class GenerationResponse(BaseModel):
    id: str
    object: str = 'text_generation'
    created: int
    model: str
    generated_text: str
    scores: float


router = APIRouter(prefix="/generate", tags=["Generations Endpoint"])


@router.post("/", response_model=GenerationResponse)
async def generate(request: GenerationRequest):
    '''
    Generates text from a locally running model.
    '''

    # global model
    if request.stream:
        raise NotImplementedError("Streaming is not yet implemented")

    params = request.dict(exclude={'model', 'prompt'})
    payload = {"inputs": request.prompt, "parameters": params}

    header = {"Content-Type": "application/json"}

    async with aiohttp.ClientSession() as session:
        try:
            async with session.post(URL, headers=header, data=json.dumps(payload)) as response:
                resp = await response.json()
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

    output = resp["generated_text"]

    return GenerationResponse(
        id=str(uuid4()),
        created=time.time(),
        model=request.model,
        generated_text=output,
        scores=resp["scores"] if "scores" in resp else -1.0,
    )
