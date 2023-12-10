import json
import hashlib
import logging
from pathlib import Path
from typing import List, Union
from fastapi import APIRouter
from api_v1.settings import settings
from gpt4all import Embed4All
from pydantic import BaseModel, Field

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Determine the root directory of the project
ROOT_DIR = Path(__file__).resolve().parent.parent  # Adjust this path based on your project structure
logger.info(f"Root directory determined as: {ROOT_DIR}")

# Use the logging inside your embedding functions
def save_embedding(input_text, embedding):
    try:
        embeddings_dir = ROOT_DIR / 'embeddings-files'
        embeddings_dir.mkdir(exist_ok=True)
        text_hash = hashlib.sha256(input_text.encode()).hexdigest()
        file_path = embeddings_dir / f'{text_hash}.json'
        with open(file_path, 'w') as file:
            json.dump({'embedding': embedding}, file)
        logger.info(f"Embedding saved to {file_path}")
    except Exception as e:
        logger.error(f"Error saving embedding: {e}")
    return file_path


def load_embedding(input_text):
    try:
        text_hash = hashlib.sha256(input_text.encode()).hexdigest()
        file_path = ROOT_DIR / f'embeddings-files/{text_hash}.json'
        with open(file_path, 'r') as file:
            data = json.load(file)
            logger.info(f"Embedding loaded from {file_path}")
            return data['embedding']
    except FileNotFoundError:
        logger.info(f"Embedding file not found for input: {input_text}")
        return None
    except Exception as e:
        logger.error(f"Error loading embedding: {e}")
        return None

# Pydantic Models
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
    file_path: str = None

# API Router
router = APIRouter(prefix="/embeddings", tags=["Embedding Endpoints"])
embedder = Embed4All()

# Endpoint Function
@router.post("/", response_model=EmbeddingResponse)
def embeddings(data: EmbeddingRequest):
    """
    Creates a GPT4All embedding
    """
    file_path=''
    # Check and handle the type of input data
    if isinstance(data.input, str):
        # Try to load existing embedding
        embedding = load_embedding(data.input)
        if not embedding:
            # If not found, generate new embedding and save it
            embedding = embedder.embed(data.input)
            file_path=save_embedding(data.input, embedding)
    elif isinstance(data.input, list):
        # Convert list to string
        input_str = ' '.join(map(str, data.input))
        # Generate new embedding for the combined string
        embedding = embedder.embed(input_str)
        file_path=save_embedding(input_str,embedding)
    else:
        raise ValueError("Invalid input type for embedding")

    return EmbeddingResponse(
        data=[Embedding(embedding=embedding)], usage=EmbeddingUsage(), model=data.model, file_path=str(file_path)
    )
