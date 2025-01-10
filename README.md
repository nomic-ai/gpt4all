<h1 align="center">GPT4All</h1>

<p align="center">
  <a href="https://www.nomic.ai/gpt4all">Website</a> &bull; <a href="https://docs.gpt4all.io">Documentation</a> &bull; <a href="https://discord.gg/mGZE39AS3e">Discord</a> &bull; <a href="https://www.youtube.com/watch?v=gQcZDXRVJok">YouTube Tutorial</a>
</p>

<p align="center">
  GPT4All runs large language models (LLMs) privately on everyday desktops & laptops.
</p>
<p align="center">
  No API calls or GPUs required - you can just download the application and <a href="https://docs.gpt4all.io/gpt4all_desktop/quickstart.html#quickstart">get started</a>.
</p>

<p align="center">
  Read about what's new in <a href="https://www.nomic.ai/blog/tag/gpt4all">our blog</a>.
</p>
<p align="center">
  <a href="https://nomic.ai/gpt4all/#newsletter-form">Subscribe to the newsletter</a>
</p>

https://github.com/nomic-ai/gpt4all/assets/70534565/513a0f15-4964-4109-89e4-4f9a9011f311

<p align="center">
GPT4All is made possible by our compute partner <a href="https://www.paperspace.com/">Paperspace</a>.
</p>

## Download Links

<p>
  &mdash; <a href="https://gpt4all.io/installers/gpt4all-installer-win64.exe">
    <img src="gpt4all-bindings/python/docs/assets/windows.png" style="height: 1em; width: auto" /> Windows Installer
  </a> &mdash;
</p>
<p>
  &mdash; <a href="https://gpt4all.io/installers/gpt4all-installer-darwin.dmg">
    <img src="gpt4all-bindings/python/docs/assets/mac.png" style="height: 1em; width: auto" /> macOS Installer
  </a> &mdash;
</p>
<p>
  &mdash; <a href="https://gpt4all.io/installers/gpt4all-installer-linux.run">
    <img src="gpt4all-bindings/python/docs/assets/ubuntu.svg" style="height: 1em; width: auto" /> Ubuntu Installer
  </a> &mdash;
</p>
<p>
  Windows and Linux require Intel Core i3 2nd Gen / AMD Bulldozer, or better. x86-64 only, no ARM.
</p>
<p>
  macOS requires Monterey 12.6 or newer. Best results with Apple Silicon M-series processors.
</p>

See the full [System Requirements](gpt4all-chat/system_requirements.md) for more details.

<br/>
<br/>
<p>
  <a href='https://flathub.org/apps/io.gpt4all.gpt4all'>
    <img style="height: 2em; width: auto" alt='Get it on Flathub' src='https://flathub.org/api/badge'><br/>
    Flathub (community maintained)
  </a>
</p>

## Install GPT4All Python

`gpt4all` gives you access to LLMs with our Python client around [`llama.cpp`](https://github.com/ggerganov/llama.cpp) implementations. 

Nomic contributes to open source software like [`llama.cpp`](https://github.com/ggerganov/llama.cpp) to make LLMs accessible and efficient **for all**.

```bash
pip install gpt4all
```

```python
from gpt4all import GPT4All
model = GPT4All("Meta-Llama-3-8B-Instruct.Q4_0.gguf") # downloads / loads a 4.66GB LLM
with model.chat_session():
    print(model.generate("How can I run LLMs efficiently on my laptop?", max_tokens=1024))
```


## Integrations

:parrot::link: [Langchain](https://python.langchain.com/v0.2/docs/integrations/providers/gpt4all/)
:card_file_box: [Weaviate Vector Database](https://github.com/weaviate/weaviate) - [module docs](https://weaviate.io/developers/weaviate/modules/retriever-vectorizer-modules/text2vec-gpt4all)
:telescope: [OpenLIT (OTel-native Monitoring)](https://github.com/openlit/openlit) - [Docs](https://docs.openlit.io/latest/integrations/gpt4all)

## Release History
- **July 2nd, 2024**: V3.0.0 Release
    - Fresh redesign of the chat application UI
    - Improved user workflow for LocalDocs
    - Expanded access to more model architectures
- **October 19th, 2023**: GGUF Support Launches with Support for:
    - Mistral 7b base model, an updated model gallery on our website, several new local code models including Rift Coder v1.5
    - [Nomic Vulkan](https://blog.nomic.ai/posts/gpt4all-gpu-inference-with-vulkan) support for Q4\_0 and Q4\_1 quantizations in GGUF.
    - Offline build support for running old versions of the GPT4All Local LLM Chat Client.
- **September 18th, 2023**: [Nomic Vulkan](https://blog.nomic.ai/posts/gpt4all-gpu-inference-with-vulkan) launches supporting local LLM inference on NVIDIA and AMD GPUs.
- **July 2023**: Stable support for LocalDocs, a feature that allows you to privately and locally chat with your data.
- **June 28th, 2023**: [Docker-based API server] launches allowing inference of local LLMs from an OpenAI-compatible HTTP endpoint.

[Docker-based API server]: https://github.com/nomic-ai/gpt4all/tree/cef74c2be20f5b697055d5b8b506861c7b997fab/gpt4all-api

## Contributing
GPT4All welcomes contributions, involvement, and discussion from the open source community!
Please see CONTRIBUTING.md and follow the issues, bug reports, and PR markdown templates.

Check project discord, with project owners, or through existing issues/PRs to avoid duplicate work.
Please make sure to tag all of the above with relevant project identifiers or your contribution could potentially get lost.
Example tags: `backend`, `bindings`, `python-bindings`, `documentation`, etc.

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
