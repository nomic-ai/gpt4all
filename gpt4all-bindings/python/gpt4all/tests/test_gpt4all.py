import sys
from io import StringIO

from gpt4all import GPT4All


def test_inference_orca():
    model = GPT4All(model_name='orca-mini-3b.ggmlv3.q4_0.bin')
    model.generate('hello')

def test_inference_falcon():
    model = GPT4All(model_name='ggml-model-gpt4all-falcon-q4_0.bin')
    model.generate('hello')
