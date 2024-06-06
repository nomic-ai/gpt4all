<h1 align="center">GPT4All</h1>
<p align="center">Privacy-oriented software for chatting with large language models that run on your own computer.</p>
<p align="center">
  <a href="https://gpt4all.io">Official Website</a> &bull; <a href="https://docs.gpt4all.io">Documentation</a> &bull; <a href="https://discord.gg/mGZE39AS3e">Discord</a>
</p>
<p align="center">
  Official Download Links: <a href="https://gpt4all.io/installers/gpt4all-installer-win64.exe">Windows</a> &mdash; <a href="https://gpt4all.io/installers/gpt4all-installer-darwin.dmg">macOS</a> &mdash; <a href="https://gpt4all.io/installers/gpt4all-installer-linux.run">Ubuntu</a>
</p>
<p align="center">
  <b>NEW:</b> <a href="https://forms.nomic.ai/gpt4all-release-notes-signup">Subscribe to our mailing list</a> for updates and news!
</p>
<p align="center">
GPT4All is made possible by our compute partner <a href="https://www.paperspace.com/">Paperspace</a>.
</p>
<p align="center">
 <a href="https://www.phorm.ai/query?projectId=755eecd3-24ad-49cc-abf4-0ab84caacf63"><img src="https://img.shields.io/badge/Phorm-Ask_AI-%23F2777A.svg" alt="phorm.ai"></a>
</p>

<p align="center">
  <img width="auto" height="400" src="https://github.com/nomic-ai/gpt4all/assets/14168726/495fce3e-769b-4e5a-a394-99f072ac4d29">
</p>
<p align="center">
Run on an M2 MacBook Pro (not sped up!)
</p>


## About GPT4All

GPT4All is an ecosystem to run **powerful** and **customized** large language models that work locally on consumer grade CPUs and NVIDIA and AMD GPUs. Note that your CPU needs to support [AVX instructions](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions).

Learn more in the [documentation](https://docs.gpt4all.io).

A GPT4All model is a 3GB - 8GB file that you can download and plug into the GPT4All software. **Nomic AI** supports and maintains this software ecosystem to enforce quality and security alongside spearheading the effort to allow any person or enterprise to easily deploy their own on-edge large language models.


### Installation

The recommended way to install GPT4All is to use one of the online installers linked above in this README, which are also available at the [GPT4All website](https://gpt4all.io/). These require an internet connection at install time, are slightly easier to use on macOS due to code signing, and provide a version of GPT4All that can check for updates.

An alternative way to install GPT4All is to use one of the offline installers available on the [Releases page](https://github.com/nomic-ai/gpt4all/releases). These do not require an internet connection at install time, and can be used to install an older version of GPT4All if so desired. But using these requires acknowledging a security warning on macOS, and they provide a version of GPT4All that is unable to notify you of updates, so you should enable notifications for Releases on this repository (Watch > Custom > Releases) or sign up for announcements in our [Discord server](https://discord.gg/mGZE39AS3e).


### What's New
- **October 19th, 2023**: GGUF Support Launches with Support for:
    - Mistral 7b base model, an updated model gallery on [gpt4all.io](https://gpt4all.io), several new local code models including Rift Coder v1.5
    - [Nomic Vulkan](https://blog.nomic.ai/posts/gpt4all-gpu-inference-with-vulkan) support for Q4\_0 and Q4\_1 quantizations in GGUF.
    - Offline build support for running old versions of the GPT4All Local LLM Chat Client.
- **September 18th, 2023**: [Nomic Vulkan](https://blog.nomic.ai/posts/gpt4all-gpu-inference-with-vulkan) launches supporting local LLM inference on NVIDIA and AMD GPUs.
- **July 2023**: Stable support for LocalDocs, a feature that allows you to privately and locally chat with your data.
- **June 28th, 2023**: [Docker-based API server] launches allowing inference of local LLMs from an OpenAI-compatible HTTP endpoint.

[Docker-based API server]: https://github.com/nomic-ai/gpt4all/tree/cef74c2be20f5b697055d5b8b506861c7b997fab/gpt4all-api


### Building From Source

* Follow the instructions [here](gpt4all-chat/build_and_run.md) to build the GPT4All Chat UI from source.


### Bindings

* :snake: <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python">Official Python Bindings</a> [![Downloads](https://static.pepy.tech/badge/gpt4all/week)](https://pepy.tech/project/gpt4all)
* :computer: <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/typescript">Typescript Bindings</a>


### Integrations

* :parrot::link: [Langchain](https://python.langchain.com/en/latest/modules/models/llms/integrations/gpt4all.html)
* :card_file_box: [Weaviate Vector Database](https://github.com/weaviate/weaviate) - [module docs](https://weaviate.io/developers/weaviate/modules/retriever-vectorizer-modules/text2vec-gpt4all)
* :telescope: [OpenLIT (OTel-native Monitoring)](https://github.com/openlit/openlit) - [Docs](https://docs.openlit.io/latest/integrations/gpt4all)


## Contributing
GPT4All welcomes contributions, involvement, and discussion from the open source community!
Please see CONTRIBUTING.md and follow the issues, bug reports, and PR markdown templates.

Check project discord, with project owners, or through existing issues/PRs to avoid duplicate work.
Please make sure to tag all of the above with relevant project identifiers or your contribution could potentially get lost.
Example tags: `backend`, `bindings`, `python-bindings`, `documentation`, etc.


## GPT4All 2024 Roadmap
To contribute to the development of any of the below roadmap items, make or find the corresponding issue and cross-reference the [in-progress task](https://github.com/orgs/nomic-ai/projects/2/views/1).

Each item should have an issue link below.

- Chat UI Language Localization (localize UI into the native languages of users)
    - [ ] Chinese
    - [ ] German
    - [ ] French
    - [ ] Portuguese
    - [ ] Your native language here. 
- UI Redesign: an internal effort at Nomic to improve the UI/UX of gpt4all for all users.
    - [ ] Design new user interface and gather community feedback
    - [ ] Implement the new user interface and experience.
- Installer and Update Improvements
    - [ ] Seamless native installation and update process on OSX
    - [ ] Seamless native installation and update process on Windows
    - [ ] Seamless native installation and update process on Linux
- Model discoverability improvements:
    - [x] Support huggingface model discoverability
    - [ ] Support Nomic hosted model discoverability
- LocalDocs (towards a local perplexity)
    - Multilingual LocalDocs Support
        - [ ] Create a multilingual experience
        - [ ] Incorporate a multilingual embedding model
        - [ ] Specify a preferred multilingual LLM for localdocs
    - Improved RAG techniques
        - [ ] Query augmentation and re-writing
        - [ ] Improved chunking and text extraction from arbitrary modalities
            - [ ] Custom PDF extractor past the QT default (charts, tables, text)
        - [ ] Faster indexing and local exact search with v1.5 hamming embeddings and reranking (skip ANN index construction!)
    - Support queries like 'summarize X document'
    - Multimodal LocalDocs support with Nomic Embed
    - Nomic Dataset Integration with real-time LocalDocs
        - [ ] Include an option to allow the export of private LocalDocs collections to Nomic Atlas for debugging data/chat quality
        - [ ] Allow optional sharing of LocalDocs collections between users.
        - [ ] Allow the import of a LocalDocs collection from an Atlas Datasets
            - Chat with live version of Wikipedia, Chat with Pubmed, chat with the latest snapshot of world news.
- First class Multilingual LLM Support
    - [ ] Recommend and set a default LLM for German
    - [ ] Recommend and set a default LLM for English
    - [ ] Recommend and set a default LLM for Chinese
    - [ ] Recommend and set a default LLM for Spanish

- Server Mode improvements
    - Improved UI and new requested features:
        - [ ] Fix outstanding bugs and feature requests around networking configurations.
        - [ ] Support Nomic Embed inferencing
        - [ ] First class documentation
        - [ ] Improving developer use and quality of server mode (e.g. support larger batches)


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
