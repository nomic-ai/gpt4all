from typing import List, Union
from fastapi import APIRouter
from api_v1.settings import settings
from gpt4all import Embed4All
from pydantic import BaseModel, Field

### This should follow https://github.com/openai/openai-openapi/blob/master/openapi.yaml


class EmbeddingRequest(BaseModel):
    model: str = Field(
        settings.model, description="The model to generate an embedding from."
    )
    input: Union[str, List[str], List[int], List[List[int]]] = Field(
        ..., description="Input text to embed, encoded as a string or array of tokens."
    )


class EmbeddingUsage(BaseModel):
    prompt_tokens: int = 0
    total_tokens: int = 0


class Embedding(BaseModel):
    index: int = 0
    object: str = "embedding"
    embedding: List[float]


class EmbeddingResponse(BaseModel):
    object: str = "list"
    model: str
    data: List[Embedding]
    usage: EmbeddingUsage


router = APIRouter(prefix="/embeddings", tags=["Embedding Endpoints"])

embedder = Embed4All()


def get_embedding(data: EmbeddingRequest) -> EmbeddingResponse:
    """
    Calculates the embedding for the given input using a specified model.

    Args:
        data (EmbeddingRequest): An EmbeddingRequest object containing the input data
        and model name.

    Returns:
        EmbeddingResponse: An EmbeddingResponse object encapsulating the calculated embedding,
        usage info, and the model name.
    """
    embedding = embedder.embed(data.input)
    return EmbeddingResponse(
        data=[Embedding(embedding=embedding)], usage=EmbeddingUsage(), model=data.model
    )


@router.post("/", response_model=EmbeddingResponse)
def embeddings(data: EmbeddingRequest):
    """
    Creates a GPT4All embedding
    """
    return get_embedding(data)
