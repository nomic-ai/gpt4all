<h1 align="center">GPT4All</h1>
<p align="center">Open-source assistant-style large language models that run locally on your CPU</p>

<p align="center">
<a href="https://gpt4all.io">GPT4All Website</a>
</p>

<p align="center">
<a href="https://docs.gpt4all.io">GPT4All Documentation</a>
</p>

<p align="center">
<a href="https://discord.gg/mGZE39AS3e">Discord</a>
</p>


<p align="center">
<a href="https://gpt4all.io/reports/GPT4All_Technical_Report_3.pdf">:green_book: Technical Report 3: GPT4All Snoozy and Groovy </a>
</p>

<p align="center">
<a href="https://static.nomic.ai/gpt4all/2023_GPT4All-J_Technical_Report_2.pdf">:green_book: Technical Report 2: GPT4All-J </a>
</p>

<p align="center">
<a href="https://s3.amazonaws.com/static.nomic.ai/gpt4all/2023_GPT4All_Technical_Report.pdf">:green_book: Technical Report 1: GPT4All</a>
</p>

<p align="center">
<a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python/README.md">:snake: Official Python Bindings</a>
</p>

<p align="center">
<a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/typescript">:computer: Official Typescript Bindings</a>
</p>

<p align="center">
<a href="https://github.com/nomic-ai/gpt4all/blob/main/gpt4all-chat/README.md">:speech_balloon: Official Chat Interface</a>
</p>

<p align="center">
<a href="https://github.com/nomic-ai/gpt4all-ui">:speech_balloon: Official Web Chat Interface</a>
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
Run on an M1 Mac (not sped up!)
</p>

## GPT4All: An ecosystem of open-source on-edge large language models.
GTP4All is an ecosystem to train and deploy **powerful** and **customized** large language models that run locally on consumer grade CPUs.

The goal is simple - be the best instruction tuned assistant-style language model that any person or enterprise can freely use, distribute and build on.

A GPT4All model is a 3GB - 8GB file that you can download and plug into the GPT4All open-source ecosystem software. **Nomic AI** supports and maintains this software ecosystem to enforce quality and security alongside spearheading the effort to allow any person or enterprise to easily train and deploy their own on-edge large language models. 


### Chat Client
Run any GPT4All model natively on your home desktop with the auto-updating desktop chat client. See <a href="https://gpt4all.io">GPT4All Website</a> for a full list of open-source models you can run with this powerful desktop application.

Direct Installer Links:

* [Mac/OSX](https://gpt4all.io/installers/gpt4all-installer-darwin.dmg)

* [Windows](https://gpt4all.io/installers/gpt4all-installer-win64.exe)

* [Ubuntu](https://gpt4all.io/installers/gpt4all-installer-linux.run)

If you have older hardware that only supports avx and not avx2 you can use these.

* [Mac/OSX - avx-only](https://gpt4all.io/installers/gpt4all-installer-darwin-avx-only.dmg)

* [Windows - avx-only](https://gpt4all.io/installers/gpt4all-installer-win64-avx-only.exe)

* [Ubuntu - avx-only](https://gpt4all.io/installers/gpt4all-installer-linux-avx-only.run)

Find the most up-to-date information on the [GPT4All Website](https://gpt4all.io/)

### Chat Client building and running

* Follow the visual instructions on the chat client [build_and_run](gpt4all-chat/build_and_run.md) page

### Bindings

* <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python/README.md">:snake: Official Python Bindings</a>
* <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/typescript">:computer: Official Typescript Bindings</a>
* <a href="https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/golang">:computer: Official GoLang Bindings</a>


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
