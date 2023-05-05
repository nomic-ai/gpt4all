# Monorepo Plan (DRAFT)

## Directory Structure
- gpt4all-api
    - RESTful API
- gpt4all-backend
    - C/C++ (ggml) model backends
- gpt4all-bindings
    - Language bindings for model backends
- gpt4all-chat
    - Chat GUI
- gpt4all-docker
    - Dockerfile recipes for various gpt4all builds
- gpt4all-training
    - Model training/inference/eval code

## Transition Plan:
This is roughly based on what's feasible now and path of least resistance.

1. Clean up gpt4all-training.
    - Remove deprecated/unneeded files
    - Organize into separate training, inference, eval, etc. directories

2. Clean up gpt4all-chat so it roughly has same structures as above 
    - Separate into gpt4all-chat and gpt4all-backends
    - Separate model backends into separate subdirectories (e.g. llama, gptj)

3. Develop Python bindings (high priority and in-flight)
    - Release Python binding as PyPi package
    - Reimplement [Nomic GPT4All](https://github.com/nomic-ai/nomic/blob/main/nomic/gpt4all/gpt4all.py#L58-L190) to call new Python bindings

4. Develop Dockerfiles for different combinations of model backends and bindings
    - Dockerfile for just model backend
    - Dockerfile for model backend and Python bindings

5. Develop RESTful API / FastAPI
