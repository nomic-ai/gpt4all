import sys
from io import StringIO

from gpt4all import GPT4All


def test_inference():
    model = GPT4All(model_name='orca-mini-3b.ggmlv3.q4_0.bin')
