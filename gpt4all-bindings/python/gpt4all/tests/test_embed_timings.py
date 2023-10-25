#!/usr/bin/env python3
import sys
import time
from io import StringIO

from gpt4all import Embed4All, GPT4All


def time_embedding(i, embedder):
    text = 'foo bar ' * i
    start_time = time.time()
    output = embedder.embed(text)
    end_time = time.time()
    elapsed_time = end_time - start_time
    print(f"Time report: {2 * i / elapsed_time} tokens/second with {2 * i} tokens taking {elapsed_time} seconds")


if __name__ == "__main__":
    embedder = Embed4All(n_threads=8)
    for i in [2**n for n in range(6, 14)]:
        time_embedding(i, embedder)
