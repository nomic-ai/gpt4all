import sys
from io import StringIO

from gpt4all import GPT4All


def test_inference():
    model = GPT4All(model_name='orca-mini-3b.ggmlv3.q4_0.bin')
    prompt = 'hello'

    print(model.generate(prompt))

    with model.chat_session():
        response = model.generate(prompt='hello')
        response = model.generate(prompt='write me a short poem')
        response = model.generate(prompt='thank you')
        print(model.current_chat_session)

    # print(model.generate(prompt))




# def test_inference_falcon():
#     model = GPT4All(model_name='ggml-model-gpt4all-falcon-q4_0.bin')
#     prompt = 'hello'
#     output = model.generate(prompt)
#     print(output)
#
#
#
# def test_inference_mpt():
#     model = GPT4All(model_name='ggml-mpt-7b-chat.bin')
#     prompt = 'hello'
#     output = model.generate(prompt)
#     print(output)
