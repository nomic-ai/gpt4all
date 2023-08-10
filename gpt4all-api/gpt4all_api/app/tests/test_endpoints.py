"""
Use the OpenAI python API to test gpt4all models.
"""
from typing import List, get_args

import openai

openai.api_base = "http://localhost:4891/v1"

openai.api_key = "not needed for a local LLM"


def test_completion():
    model = "ggml-mpt-7b-chat.bin"
    prompt = "Who is Michael Jordan?"
    response = openai.Completion.create(
        model=model, prompt=prompt, max_tokens=50, temperature=0.28, top_p=0.95, n=1, echo=True, stream=False
    )
    assert len(response['choices'][0]['text']) > len(prompt)

def test_streaming_completion():
    model = "ggml-mpt-7b-chat.bin"
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


def test_batched_completion():
    model = "ggml-mpt-7b-chat.bin"
    prompt = "Who is Michael Jordan?"
    response = openai.Completion.create(
        model=model, prompt=[prompt] * 3, max_tokens=50, temperature=0.28, top_p=0.95, n=1, echo=True, stream=False
    )
    assert len(response['choices'][0]['text']) > len(prompt)
    assert len(response['choices']) == 3


def test_embedding():
    model = "ggml-all-MiniLM-L6-v2-f16.bin"
    prompt = "Who is Michael Jordan?"
    response = openai.Embedding.create(model=model, input=prompt)
    output = response["data"][0]["embedding"]
    args = get_args(List[float])

    assert response["model"] == model
    assert isinstance(output, list)
    assert all(isinstance(x, args) for x in output)
