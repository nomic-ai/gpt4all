import pytest

from gpt4all.gpt4all import GPT4All

def test_invalid_model_type():
    model_type = "bad_type"
    with pytest.raises(ValueError):
        GPT4All.get_model_from_type(model_type)

def test_valid_model_type():
    model_type = "gptj"
    assert GPT4All.get_model_from_type(model_type).model_type == model_type

def test_invalid_model_name():
    model_name = "bad_filename.bin"
    with pytest.raises(ValueError):
        GPT4All.get_model_from_name(model_name)

def test_valid_model_name():
    model_name = "ggml-gpt4all-l13b-snoozy"
    model_type = "llama"
    assert GPT4All.get_model_from_name(model_name).model_type == model_type
    model_name += ".bin"
    assert GPT4All.get_model_from_name(model_name).model_type == model_type

def test_build_prompt():
    messages = [
        {
            "role": "system",
            "content": "You are a helpful assistant."
        },
        {
            "role": "user",
            "content": "Hello there."
        },
        {
            "role": "assistant",
            "content": "Hi, how can I help you?"
        },
        {
            "role": "user",
            "content": "Reverse a list in Python."
        }
    ]

    expected_prompt = """You are a helpful assistant.\
    \n### Instruction:
            The prompt below is a question to answer, a task to complete, or a conversation 
            to respond to; decide which and write an appropriate response.\
    ### Prompt:\
    Hello there.\
    Response: Hi, how can I help you?\
    Reverse a list in Python.\
    ### Response:"""

    print(expected_prompt)

    full_prompt = GPT4All._build_prompt(messages, default_prompt_footer=True, default_prompt_header=True)

    print("\n\n\n")
    print(full_prompt)
    assert len(full_prompt) == len(expected_prompt)
