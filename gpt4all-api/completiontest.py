# Create test batched completion function with OpenAI API

import openai
import os
from dotenv import load_dotenv

# Set OpenAI API base and key
openai.api_base = "http://localhost:4891/v1"
# openai.api_key = "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"
openai.api_key = "not needed for a local LLM"

# Load the .env file
env_path = 'gpt4all-api/gpt4all_api/.env'
load_dotenv(dotenv_path=env_path)

# Fetch MODEL_ID from .env file
model_id = os.getenv('MODEL_BIN', 'default_model_id')
embedding = os.getenv('EMBEDDING', 'default_embedding_model_id')
print (model_id)
print (embedding)

prompt = "Who is Michael Jordan?"
response = openai.Completion.create(model=model_id,
                                    prompt=[prompt] * 3,
                                    max_tokens=50,
                                    temperature=0.28,
                                    top_p=0.95,
                                    n=1,
                                    echo=True,
                                    stream=False
    )
assert len(response['choices'][0]['text']) > len(prompt)
assert len(response['choices']) == 3                                    
print(response['choices'][0]['text'])