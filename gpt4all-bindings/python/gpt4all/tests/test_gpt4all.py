import sys
from io import StringIO

from gpt4all import GPT4All


def test_inference():
    model = GPT4All(model_name='orca-mini-3b.ggmlv3.q4_0.bin')
    output_1 = model.generate('hello', top_k=1)

    with model.chat_session():
        response = model.generate(prompt='hello', top_k=1)
        response = model.generate(prompt='write me a short poem', top_k=1)
        response = model.generate(prompt='thank you', top_k=1)
        print(model.current_chat_session)

    output_2 = model.generate('hello', top_k=1)

    assert output_1 == output_2

    tokens = []
    for token in model.generate('hello', streaming=True):
        tokens.append(token)

    assert len(tokens) > 0

    with model.chat_session():
        try:
            response = model.generate(prompt='hello', top_k=1, streaming=True)
            assert False
        except NotImplementedError:
            assert True



def test_inference_hparams():
    model = GPT4All(model_name='orca-mini-3b.ggmlv3.q4_0.bin')

    output = model.generate("The capital of france is ", max_tokens=3)
    assert 'Paris' in output




def test_inference_falcon():
    model = GPT4All(model_name='ggml-model-gpt4all-falcon-q4_0.bin')
    prompt = 'hello'
    output = model.generate(prompt)

    assert len(output) > 0

def test_inference_mpt():
    model = GPT4All(model_name='ggml-mpt-7b-chat.bin')
    prompt = 'hello'
    output = model.generate(prompt)
    assert len(output) > 0
