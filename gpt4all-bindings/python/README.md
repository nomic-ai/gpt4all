# Python GPT4All

This package contains a set of Python bindings around the `llmodel` C-API.

## Documentation
docs.gpt4all.io

## Installation

```
pip install gpt4all
```

## Local Build Instructions

**NOTE**: If you are doing this on a Windows machine, you must build the GPT4All backend using [MinGW64](https://www.mingw-w64.org/) compiler.

1. Setup `llmodel`

```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all
cd gpt4all/gpt4all-backend/
mkdir build
cd build
cmake ..
cmake --build . --parallel
```
Confirm that `libllmodel.*` exists in `gpt4all-backend/build`.

2. Setup Python package

```
cd ../../gpt4all-bindings/python
pip3 install -e .
```

3. Test it out! In a Python script or console:

```python

from gpt4all import GPT4All

gptj = GPT4All("ggml-gpt4all-j-v1.3-groovy")
messages = [{"role": "user", "content": "Name 3 colors"}]
gptj.chat_completion(messages)

```
