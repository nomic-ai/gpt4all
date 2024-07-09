# Frequently Asked Questions

## Models

### Which language models are supported?

We support models with a `llama.cpp` implementation which have been uploaded to [HuggingFace](https://huggingface.co/).

### Which embedding models are supported?

We support SBert and Nomic Embed Text v1 & v1.5.

## Software

### What software do I need?

All you need is to [install GPT4all](../index.md) onto you Windows, Mac, or Linux computer.

### Which SDK languages are supported?

Our SDK is in Python for usability, but these are light bindings around [`llama.cpp`](https://github.com/ggerganov/llama.cpp) implementations that we contribute to for efficiency and accessibility on everyday computers.

### Is there an API?

Yes, you can run your model in server-mode with our [OpenAI-compatible API](https://platform.openai.com/docs/api-reference/completions), which you can configure in [settings](../gpt4all_desktop/settings.md#application-settings)

### Can I monitor a GPT4All deployment?

Yes, GPT4All [integrates](../gpt4all_python/monitoring.md) with [OpenLIT](https://github.com/openlit/openlit) so you can deploy LLMs with user interactions and hardware usage automatically monitored for full observability.

### Is there a command line interface (CLI)?

[Yes](https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/cli), we have a lightweight use of the Python client as a CLI. We welcome further contributions!

## Hardware

### What hardware do I need?

GPT4All can run on CPU, Metal (Apple Silicon M1+), and GPU.

### What are the system requirements?

Your CPU needs to support [AVX or AVX2 instructions](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions) and you need enough RAM to load a model into memory.
