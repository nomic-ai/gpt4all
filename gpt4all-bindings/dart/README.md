# GPT4All Dart Binding

## Getting started

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
