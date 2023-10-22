# GPT4All
Welcome to the GPT4All technical documentation.

GPT4All is an open-source software ecosystem that allows anyone to train and deploy **powerful** and **customized** large language models (LLMs) on **everyday hardware**.
Nomic AI oversees contributions to the open-source ecosystem ensuring quality, security and maintainability.

GPT4All software is optimized to run inference of 3-13 billion parameter large language models on the CPUs of laptops, desktops and servers.

=== "GPT4All Example"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All("orca-mini-3b-gguf2-q4_0.gguf")
    output = model.generate("The capital of France is ", max_tokens=3)
    print(output)
    ```
=== "Output"
    ```
    1. Paris
    ```
See [Python Bindings](gpt4all_python.md) to use GPT4All.

### Navigating the Documentation
In an effort to ensure cross-operating-system and cross-language compatibility, the [GPT4All software ecosystem](https://github.com/nomic-ai/gpt4all)
is organized as a monorepo with the following structure:

- **gpt4all-backend**: The GPT4All backend maintains and exposes a universal, performance optimized C API for running inference with multi-billion parameter Transformer Decoders.
This C API is then bound to any higher level programming language such as C++, Python, Go, etc.
- **gpt4all-bindings**: GPT4All bindings contain a variety of high-level programming languages that implement the C API. Each directory is a bound programming language. The [CLI](gpt4all_cli.md) is included here, as well.
- **gpt4all-api**: The GPT4All API (under initial development) exposes REST API endpoints for gathering completions and embeddings from large language models.
- **gpt4all-chat**: GPT4All Chat is an OS native chat application that runs on macOS, Windows and Linux. It is the easiest way to run local, privacy aware chat assistants on everyday hardware. You can download it on the [GPT4All Website](https://gpt4all.io) and read its source code in the monorepo.

Explore detailed documentation for the backend, bindings and chat client in the sidebar.
## Models
The GPT4All software ecosystem is compatible with the following Transformer architectures:

- `Falcon`
- `LLaMA` (including `OpenLLaMA`)
- `MPT` (including `Replit`)
- `GPT-J`

You can find an exhaustive list of supported models on the [website](https://gpt4all.io) or in the [models directory](https://raw.githubusercontent.com/nomic-ai/gpt4all/main/gpt4all-chat/metadata/models2.json)


GPT4All models are artifacts produced through a process known as neural network quantization.
A multi-billion parameter Transformer Decoder usually takes 30+ GB of VRAM to execute a forward pass.
Most people do not have such a powerful computer or access to GPU hardware. By running trained LLMs through quantization algorithms, 
some GPT4All models can run on your laptop using only 4-8GB of RAM enabling their wide-spread usage.
Bigger models might still require more RAM, however.

Any model trained with one of these architectures can be quantized and run locally with all GPT4All bindings and in the
chat client. You can add new variants by contributing to the gpt4all-backend.

## Frequently Asked Questions
Find answers to frequently asked questions by searching the [Github issues](https://github.com/nomic-ai/gpt4all/issues) or in the [documentation FAQ](gpt4all_faq.md).

## Getting the most of your local LLM

**Inference Speed**
of a local LLM depends on two factors: model size and the number of tokens given as input. 
It is not advised to prompt local LLMs with large chunks of context as their inference speed will heavily degrade.
You will likely want to run GPT4All models on GPU if you would like to utilize context windows larger than 750 tokens. Native GPU support for GPT4All models is planned.

**Inference Performance:**
Which model is best? That question depends on your use-case. The ability of an LLM to faithfully follow instructions is conditioned
on the quantity and diversity of the pre-training data it trained on and the diversity, quality and factuality of the data the LLM
was fine-tuned on. A goal of GPT4All is to bring the most powerful local assistant model to your desktop and Nomic AI is actively
working on efforts to improve their performance and quality.
