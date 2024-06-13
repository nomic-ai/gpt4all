# Python GPT4All

This package contains a set of Python bindings around the `llmodel` C-API.

Package on PyPI: https://pypi.org/project/gpt4all/

## Documentation
https://docs.gpt4all.io/gpt4all_python.html

## Installation

The easiest way to install the Python bindings for GPT4All is to use pip:

```
pip install gpt4all
```

This will download the latest version of the `gpt4all` package from PyPI.

## Local Build

As an alternative to downloading via pip, you may build the Python bindings from source.

### Prerequisites

You will need a compiler. On Windows, you should install Visual Studio with the C++ Development components. On macOS, you will need the full version of Xcode&mdash;Xcode Command Line Tools lacks certain required tools. On Linux, you will need a GCC or Clang toolchain with C++ support.

On Windows and Linux, building GPT4All with full GPU support requires the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) and the latest [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads).

### Building the python bindings

1. Clone GPT4All and change directory:
```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all.git
cd gpt4all/gpt4all-backend
```

2. Build the backend.

If you are using Windows and have Visual Studio installed:
```
cmake -B build
cmake --build build --parallel --config RelWithDebInfo
```

For all other platforms:
```
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

`RelWithDebInfo` is a good default, but you can also use `Release` or `Debug` depending on the situation.

2. Install the Python package:
```
cd ../gpt4all-bindings/python
pip install -e .
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
