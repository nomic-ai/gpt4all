import logging
import time
from typing import List
from uuid import uuid4
from fastapi import APIRouter
from pydantic import BaseModel, Field
from api_v1.settings import settings
from fastapi.responses import StreamingResponse
from gpt4all import GPT4All  # Import GPT4All
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
import json
import os
from starlette.responses import StreamingResponse
import asyncio

### This should follow https://github.com/openai/openai-openapi/blob/master/openapi.yaml
class ChatCompletionMessage(BaseModel):
    role: str
    content: str

class ChatCompletionRequest(BaseModel):
    session_id: str = Field(default_factory=lambda: str(uuid4()), description="Unique identifier for the chat session.")
    model: str = Field(settings.model, description='The model to generate a completion from.')
    messages: List[ChatCompletionMessage] = Field(..., description='Messages for the chat completion.')
    stream: bool = False  # Field for streaming
    temperature: float = Field(0.7, description='Model temperature for creativity.')  # New field
    top_p: float = Field(0.1, description='Top p sampling parameter.')  # New field
    max_tokens = Field(200,description='Number max of tokens') #New field
    repeat_penalty = Field(1.3, description='Penalty for repeating')


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


class CompletionStreamResponse(BaseModel):
    id: str
    object: str = 'text_completion'
    created: int
    model: str
    choices: List[ChatCompletionChoice]

router = APIRouter(prefix="/chat", tags=["Completions Endpoints"])

# Add a function to save the chat session
def save_chat_session(session_id, data):
    os.makedirs('chat_sessions', exist_ok=True)
    with open(f'chat_sessions/{session_id}.json', 'w') as file:
        json.dump(data, file)

def load_chat_session(session_id):
    try:
        with open(f'chat_sessions/{session_id}.json', 'r') as file:
            content = file.read()
            if not content:
                logger.error(f"Empty or invalid content in session file: {session_id}")
                return []  # Return an empty list if file is empty or invalid
            return json.loads(content)
    except FileNotFoundError:
        logger.error(f"File not found for session: {session_id}")
        return []  # Return an empty list if file not found
    except json.JSONDecodeError as e:
        logger.error(f"JSON decode error for session {session_id}: {str(e)}")
        return []  # Return an empty list if JSON is invalid


@router.get("/completions/final")
async def final_completion(session_id: str):
    '''
    Return final conversation answers
    '''
    chat_history = load_chat_session(session_id)
    return chat_history

# Initialize GPT4All model outside of the endpoint function
gpt_model = GPT4All(model_name=settings.model, model_path=settings.gpt4all_path)
print("MODEL: "+settings.model)
@router.post("/completions", response_model=ChatCompletionResponse)
async def chat_completion(request: ChatCompletionRequest):
    '''
    Generates a GPT4All model response based on the conversation history.
    '''
    chat_history = load_chat_session(request.session_id)
    logger.info(request.messages[-1])
    chat_history.append({"role":request.messages[-1].role,"content":request.messages[-1].content})
    save_chat_session(request.session_id, chat_history)
    # Prepare the prompt from the incoming messages
    prompt = "\n".join(f"{msg.role}: {msg.content}" for msg in request.messages)

    # Generation parameters
    generation_params = {
        "max_tokens": request.max_tokens,  # Adjust as needed
        "temp": request.temperature,
        "top_p": request.top_p,
	    "repeat_penalty":request.repeat_penalty
    }

    try:
        with gpt_model.chat_session() as chat_session:
            # Check if streaming is requested
            if request.stream:
                async def generate_stream():
                    with gpt_model.chat_session() as chat_session:
                        cumulative_output=''
                        for output in chat_session.generate(prompt, streaming=True, **generation_params):
                            # Break the output into words
                            logger.info("OUTPUT: "+output)

                            cumulative_output=cumulative_output+output
                            logger.info("CUMULATIVE_OUTPUT: "+cumulative_output)
                            # Create a JSON object for each word
                            yield f"data: {cumulative_output}\n\n"

                        # Save the chat session at the end
                        chat_history.append({"role": "system", "content": cumulative_output})
                        save_chat_session(request.session_id, chat_history)


                return StreamingResponse(generate_stream(), media_type="text/event-stream")


            # Non-streaming response
            else:
                response_content = chat_session.generate(prompt, **generation_params)
                response_message = ChatCompletionMessage(role="system", content=response_content)
                response_choice = ChatCompletionChoice(
                    message=response_message,
                    index=0,
                    logprobs=-1.0,
                    finish_reason="length"
                )
                chat_response = ChatCompletionResponse(
                    id=str(uuid4()),
                    created=int(time.time()),
                    model=request.model,
                    choices=[response_choice],
                    usage=ChatCompletionUsage(prompt_tokens=0, completion_tokens=0, total_tokens=0)
                )
                # Save the updated chat history
                chat_history.append(response_message.content)
                save_chat_session(request.session_id, chat_history)
                return chat_response

    except Exception as e:
        logger.error(f"Error in response generation: {str(e)}")
        raise HTTPException(status_code=500, detail="Error in generating response")
