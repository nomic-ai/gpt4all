import logging
import time
from typing import List
from uuid import uuid4
from fastapi import APIRouter, HTTPException
from gpt4all import GPT4All
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
    temperature: float = Field(settings.temp, description='Model temperature')

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
    # GPU is not implemented yet
    if settings.inference_mode == "gpu":
        raise HTTPException(status_code=400,
              detail=f"Not implemented yet: Can only infer in CPU mode.")

    # we only support the configured model
    if request.model != settings.model:
        raise HTTPException(status_code=400,
              detail=f"The GPT4All inference server is booted to only infer: `{settings.model}`")

    # run only of we have a message
    if request.messages:
        model = GPT4All(model_name=settings.model, model_path=settings.gpt4all_path)

        # format system message and conversation history correctly
        formatted_messages = ""
        for message in request.messages:
            formatted_messages += f"<|im_start|>{message.role}\n{message.content}<|im_end|>\n"

        # the LLM will complete the response of the assistant
        formatted_messages += "<|im_start|>assistant\n"
        response = model.generate(
            prompt=formatted_messages,
            temp=request.temperature
            )

        # the LLM may continue to hallucinate the conversation, but we want only the first response
        # so, cut off everything after first <|im_end|>
        index = response.find("<|im_end|>")
        response_content = response[:index].strip()
    else:
        response_content = "No messages received."

    # Create a chat message for the response
    response_message = ChatCompletionMessage(role="assistant", content=response_content)

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
