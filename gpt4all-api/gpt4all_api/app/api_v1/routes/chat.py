import logging
import time
from typing import List
from uuid import uuid4
from fastapi import APIRouter
from pydantic import BaseModel, Field
from api_v1.settings import settings
from fastapi.responses import StreamingResponse

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

### This should follow https://github.com/openai/openai-openapi/blob/master/openapi.yaml
class ChatCompletionMessage(BaseModel):
    role: str
    content: str

class ChatCompletionRequest(BaseModel):
    model: str = Field(settings.model, description='The model to generate a completion from.')
    messages: List[ChatCompletionMessage] = Field(..., description='Messages for the chat completion.')

class ChatCompletionChoice(BaseModel):
    message: ChatCompletionMessage
    index: int
    logprobs: float
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
    Completes a GPT4All model response based on the last message in the chat.
    '''
    # Example: Echo the last message content with some modification
    if request.messages:
        last_message = request.messages[-1].content
        response_content = f"Echo: {last_message}"
    else:
        response_content = "No messages received."

    # Create a chat message for the response
    response_message = ChatCompletionMessage(role="system", content=response_content)

    # Create a choice object with the response message
    response_choice = ChatCompletionChoice(
        message=response_message,
        index=0,
        logprobs=-1.0,  # Placeholder value
        finish_reason="length"  # Placeholder value
    )

    # Create the response object
    chat_response = ChatCompletionResponse(
        id=str(uuid4()),
        created=int(time.time()),
        model=request.model,
        choices=[response_choice],
        usage=ChatCompletionUsage(prompt_tokens=0, completion_tokens=0, total_tokens=0),  # Placeholder values
    )

    return chat_response
