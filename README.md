<h1 align="center">GPT4All</h1>

<p align="center">Open-source large language models that run locally on your CPU and nearly any GPU</p>

<p align="center">
<a href="https://gpt4all.io">GPT4All Website and Models</a>
</p>

<p align="center">
<a href="https://docs.gpt4all.io">GPT4All Documentation</a>
</p>

<p align="center">
<a href="https://discord.gg/mGZE39AS3e">Discord</a>
</p>

<p align="center">
<a href="https://python.langchain.com/en/latest/modules/models/llms/integrations/gpt4all.html">🦜️🔗 Official Langchain Backend</a> 
</p>

<p align="center">
GPT4All is made possible by our compute partner <a href="https://www.paperspace.com/">Paperspace</a>.
</p>

<p align="center">
  <img width="600" height="365" src="https://user-images.githubusercontent.com/13879686/231876409-e3de1934-93bb-4b4b-9013-b491a969ebbc.gif">
</p>
<p align="center">
Run on an M1 macOS Device (not sped up!)
</p>

## GPT4All: An ecosystem of open-source on-edge large language models.

> [!IMPORTANT]
> GPT4All v2.5.0 and newer only supports models in GGUF format (.gguf). Models used with a previous version of GPT4All (.bin extension) will no longer work.

GPT4All is an ecosystem to run **powerful** and **customized** large language models that work locally on consumer grade CPUs and any GPU. Note that your CPU needs to support [AVX or AVX2 instructions](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions).

Learn more in the [documentation](https://docs.gpt4all.io).

A GPT4All model is a 3GB - 8GB file that you can download and plug into the GPT4All open-source ecosystem software. **Nomic AI** supports and maintains this software ecosystem to enforce quality and security alongside spearheading the effort to allow any person or enterprise to easily train and deploy their own on-edge large language models.

### What's New ([Issue Tracker](https://github.com/orgs/nomic-ai/projects/2))
- **October 19th, 2023**: GGUF Support Launches with Support for:
    - Mistral 7b base model, an updated model gallery on [gpt4all.io](https://gpt4all.io), several new local code models including Rift Coder v1.5
    - [Nomic Vulkan](https://blog.nomic.ai/posts/gpt4all-gpu-inference-with-vulkan) support for Q4_0, Q6 quantizations in GGUF.
    - Offline build support for running old versions of the GPT4All Local LLM Chat Client.
- **September 18th, 2023**: [Nomic Vulkan](https://blog.nomic.ai/posts/gpt4all-gpu-inference-with-vulkan) launches supporting local LLM inference on AMD, Intel, Samsung, Qualcomm and NVIDIA GPUs.
- **August 15th, 2023**: GPT4All API launches allowing inference of local LLMs from docker containers.
- **July 2023**: Stable support for LocalDocs, a GPT4All Plugin that allows you to privately and locally chat with your data.


### Chat Client
Run any GPT4All model natively on your home desktop with the auto-updating desktop chat client. See <a href="https://gpt4all.io">GPT4All Website</a> for a full list of open-source models you can run with this powerful desktop application.

Direct Installer Links:

* [macOS](https://gpt4all.io/installers/gpt4all-installer-darwin.dmg)

* [Windows](https://gpt4all.io/installers/gpt4all-installer-win64.exe)

* [Ubuntu](https://gpt4all.io/installers/gpt4all-installer-linux.run)

Find the most up-to-date information on the [GPT4All Website](https://gpt4all.io/)

### Chat Client building and running

* Follow the visual instructions on the chat client [build_and_run](gpt4all-chat/build_and_run.md) page

### Bindings

* <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python/README.md">:snake: Official Python Bindings</a> [![Downloads](https://static.pepy.tech/badge/gpt4all/week)](https://pepy.tech/project/gpt4all)
* <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/typescript">:computer: Official Typescript Bindings</a>
* <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/golang">:computer: Official GoLang Bindings</a>
* <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/csharp">:computer: Official C# Bindings</a>
* <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/java">:computer: Official Java Bindings</a>

### Integrations

* 🗃️ [Weaviate Vector Database](https://github.com/weaviate/weaviate) - [module docs](https://weaviate.io/developers/weaviate/modules/retriever-vectorizer-modules/text2vec-gpt4all)

## Contributing
GPT4All welcomes contributions, involvement, and discussion from the open source community!
Please see CONTRIBUTING.md and follow the issues, bug reports, and PR markdown templates.

Check project discord, with project owners, or through existing issues/PRs to avoid duplicate work.
Please make sure to tag all of the above with relevant project identifiers or your contribution could potentially get lost.
Example tags: `backend`, `bindings`, `python-bindings`, `documentation`, etc.

## Technical Reports

<p align="center">
<a href="https://gpt4all.io/reports/GPT4All_Technical_Report_3.pdf">:green_book: Technical Report 3: GPT4All Snoozy and Groovy </a>
</p>

<p align="center">
<a href="https://static.nomic.ai/gpt4all/2023_GPT4All-J_Technical_Report_2.pdf">:green_book: Technical Report 2: GPT4All-J </a>
</p>

<p align="center">
<a href="https://s3.amazonaws.com/static.nomic.ai/gpt4all/2023_GPT4All_Technical_Report.pdf">:green_book: Technical Report 1: GPT4All</a>
</p>

## Citation

If you utilize this repository, models or data in a downstream project, please consider citing it with:
```
@misc{gpt4all,
  author = {Yuvanesh Anand and Zach Nussbaum and Brandon Duderstadt and Benjamin Schmidt and Andriy Mulyar},
  title = {GPT4All: Training an Assistant-style Chatbot with Large Scale Data Distillation from GPT-3.5-Turbo},
  year = {2023},
  publisher = {GitHub},
  journal = {GitHub repository},
  howpublished = {\url{https://github.com/nomic-ai/gpt4all}},
}
```
