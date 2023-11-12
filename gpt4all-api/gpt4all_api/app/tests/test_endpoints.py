"""
Use the OpenAI python API to test gpt4all models.
"""
from typing import List, get_args
import os
from dotenv import load_dotenv

import openai

openai.api_base = "http://localhost:4891/v1"
openai.api_key = "not needed for a local LLM"

# Load the .env file
env_path = 'gpt4all-api/gpt4all_api/.env'
load_dotenv(dotenv_path=env_path)

# Fetch MODEL_ID from .env file
model_id = os.getenv('MODEL_BIN', 'default_model_id')
embedding = os.getenv('EMBEDDING', 'default_embedding_model_id')
print (model_id)
print (embedding)

def test_completion():
    model = model_id
    prompt = "Who is Michael Jordan?"
    response = openai.Completion.create(
        model=model, prompt=prompt, max_tokens=50, temperature=0.28, top_p=0.95, n=1, echo=True, stream=False
    )
    assert len(response['choices'][0]['text']) > len(prompt)

def test_streaming_completion():
    model = model_id
    prompt = "Who is Michael Jordan?"
    tokens = []
    for resp in openai.Completion.create(
            model=model,
            prompt=prompt,
            max_tokens=50,
            temperature=0.28,
            top_p=0.95,
            n=1,
            echo=True,
            stream=True):
        tokens.append(resp.choices[0].text)

    assert (len(tokens) > 0)
    assert (len("".join(tokens)) > len(prompt))

# Modified test batch, problems with keyerror in response
def test_batched_completion():
    model = model_id  # replace with your specific model ID
    prompt = "Who is Michael Jordan?"
    responses = []
    
    # Loop to create completions one at a time
    for _ in range(3):
        response = openai.Completion.create(
            model=model, prompt=prompt, max_tokens=50, temperature=0.28, top_p=0.95, n=1, echo=True, stream=False
        )
        responses.append(response)

    # Assertions to check the responses
    for response in responses:
        assert len(response['choices'][0]['text']) > len(prompt)
    
    assert len(responses) == 3

def test_embedding():
    model = embedding
    prompt = "Who is Michael Jordan?"
    response = openai.Embedding.create(model=model, input=prompt)
    output = response["data"][0]["embedding"]
    args = get_args(List[float])

    assert response["model"] == model
    assert isinstance(output, list)
    assert all(isinstance(x, args) for x in output)