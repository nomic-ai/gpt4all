# Python GPT4All

This package contains a set of Python bindings around the `llmodel` C-API.

Package on PyPI: https://pypi.org/project/gpt4all/

## Documentation
https://docs.gpt4all.io/gpt4all_python.html

## Installation

```
pip install gpt4all
```

## Local Build Instructions

### Prerequisites

On Windows and Linux, building GPT4All requires the complete Vulkan SDK. You may download it from here: https://vulkan.lunarg.com/sdk/home

macOS users do not need Vulkan, as GPT4All will use Metal instead.

### Building the python bindings

**NOTE**: If you are doing this on a Windows machine, you must build the GPT4All backend using [MinGW64](https://www.mingw-w64.org/) compiler.

1. Setup `llmodel`

```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all.git
cd gpt4all/gpt4all-backend/
mkdir build
cd build
cmake ..
cmake --build . --parallel  # optionally append: --config Release
```
Confirm that `libllmodel.*` exists in `gpt4all-backend/build`.

2. Setup Python package

```
cd ../../gpt4all-bindings/python
pip3 install -e .
```

## Usage

Test it out! In a Python script or console:

```python
from gpt4all import GPT4All
model = GPT4All("orca-mini-3b-gguf2-q4_0.gguf")
output = model.generate("The capital of France is ", max_tokens=3)
print(output)
```


GPU Usage
```python
from gpt4all import GPT4All
model = GPT4All("orca-mini-3b-gguf2-q4_0.gguf", device='gpu') # device='amd', device='intel'
output = model.generate("The capital of France is ", max_tokens=3)
print(output)
```

## Troubleshooting a Local Build
- If you're on Windows and have compiled with a MinGW toolchain, you might run into an error like:
  ```
  FileNotFoundError: Could not find module '<...>\gpt4all-bindings\python\gpt4all\llmodel_DO_NOT_MODIFY\build\libllmodel.dll'
  (or one of its dependencies). Try using the full path with constructor syntax.
  ```
  The key phrase in this case is _"or one of its dependencies"_. The Python interpreter you're using
  probably doesn't see the MinGW runtime dependencies. At the moment, the following three are required:
  `libgcc_s_seh-1.dll`, `libstdc++-6.dll` and `libwinpthread-1.dll`. You should copy them from MinGW
  into a folder where Python will see them, preferably next to `libllmodel.dll`.

- Note regarding the Microsoft toolchain: Compiling with MSVC is possible, but not the official way to
  go about it at the moment. MSVC doesn't produce DLLs with a `lib` prefix, which the bindings expect.
  You'd have to amend that yourself.
