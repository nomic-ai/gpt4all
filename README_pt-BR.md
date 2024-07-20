<div align="center">

[English](README.md) | **Português (Brasil)**

</div>

<h1 align="center">GPT4All</h1>

<p align="center">O GPT4All executa grandes modelos de linguagem (LLMs) em desktops e laptops comuns de forma local e privada. <br> <br> Sem necessidade de chamadas de API ou GPUs - basta baixar o aplicativo e <a href="https://docs.gpt4all.io/gpt4all_desktop/quickstart.html#quickstart">começar</a>

https://github.com/nomic-ai/gpt4all/assets/70534565/513a0f15-4964-4109-89e4-4f9a9011f311

<p align="center">
  <a href="https://gpt4all.io/installers/gpt4all-installer-win64.exe">
    <img src="gpt4all-bindings/python/docs/assets/windows.png" width="80" height="80"><br>
    Baixar para Windows
  </a>
</p>

<p align="center">
  <a href="https://gpt4all.io/installers/gpt4all-installer-darwin.dmg">
    <img src="gpt4all-bindings/python/docs/assets/mac.png" width="85" height="100"><br>
    Baixar para MacOS
  </a>
</p>

<p align="center">
  <a href="https://gpt4all.io/installers/gpt4all-installer-linux.run">
    <img src="gpt4all-bindings/python/docs/assets/ubuntu.svg" width="120" height="120"><br>
    Baixar para Ubuntu
  </a>
</p>

<p align="center">
  <a href="https://gpt4all.io">Site</a> &bull; <a href="https://docs.gpt4all.io">Documentação</a> &bull; <a href="https://discord.gg/mGZE39AS3e">Discord</a>
</p>
<p align="center">
  <a href="https://forms.nomic.ai/gpt4all-release-notes-signup">Assine a newsletter</a>
</p>
<p align="center">
O GPT4All é possível graças ao nosso parceiro <a href="https://www.paperspace.com/">Paperspace</a>.
</p>
<p align="center">
 <a href="https://www.phorm.ai/query?projectId=755eecd3-24ad-49cc-abf4-0ab84caacf63"><img src="https://img.shields.io/badge/Phorm-Ask_AI-%23F2777A.svg" alt="phorm.ai"></a>
</p>

## Instalar o GPT4All para Python

O `gpt4all` oferece acesso a LLMs com nosso cliente Python em torno de implementações do [`llama.cpp`](https://github.com/ggerganov/llama.cpp).

A Nomic contribui com software de código aberto como [`llama.cpp`](https://github.com/ggerganov/llama.cpp) para tornar os LLMs acessíveis e eficientes **para todos**.

```bash
pip install gpt4all
```

```python
from gpt4all import GPT4All
model = GPT4All("Meta-Llama-3-8B-Instruct.Q4_0.gguf") # baixa / carrega um LLM de 4.66GB
with model.chat_session():
    print(model.generate("Como posso executar LLMs de forma eficiente no meu laptop?", max_tokens=1024))
```


## Integrações

:parrot::link: [Langchain](https://python.langchain.com/v0.2/docs/integrations/providers/gpt4all/)
:card_file_box: [Weaviate Vector Database](https://github.com/weaviate/weaviate) - [documentação do módulo](https://weaviate.io/developers/weaviate/modules/retriever-vectorizer-modules/text2vec-gpt4all)
:telescope: [OpenLIT (Monitoramento nativo OTel)](https://github.com/openlit/openlit) - [Documentação](https://docs.openlit.io/latest/integrations/gpt4all)

## Histórico de Versões
- **2 de julho de 2024**: Lançamento da versão 3.0.0
    - Novo design da interface do aplicativo de chat.
    - Fluxo de trabalho aprimorado para o LocalDocs.
    - Acesso expandido a mais arquiteturas de modelo.
- **19 de outubro de 2023**: Lançamento do suporte a GGUF com:
    - Modelo base Mistral 7b, uma galeria de modelos atualizada em [gpt4all.io](https://gpt4all.io) e vários novos modelos de código local, incluindo Rift Coder v1.5.
    - Suporte ao [Nomic Vulkan](https://blog.nomic.ai/posts/gpt4all-gpu-inference-with-vulkan) para quantizações Q4\_0 e Q4\_1 em GGUF.
    - Suporte à compilação offline para executar versões antigas do cliente de chat GPT4All Local LLM.
- **18 de setembro de 2023**: Lançamento do [Nomic Vulkan](https://blog.nomic.ai/posts/gpt4all-gpu-inference-with-vulkan) com suporte para inferência local de LLMs em GPUs NVIDIA e AMD.
- **Julho de 2023**: Suporte para o LocalDocs, um recurso que permite conversas privadas e locais com seus próprios dados.
- **28 de junho de 2023**: Lançamento do [Servidor de API baseado em Docker], permitindo a inferência de LLMs locais a partir de um endpoint HTTP compatível com OpenAI.

[Servidor API baseado em Docker]: https://github.com/nomic-ai/gpt4all/tree/cef74c2be20f5b697055d5b8b506861c7b997fab/gpt4all-api

## Contribuindo
O GPT4All aceita contribuições, participação e comentários da comunidade de código aberto!
Por favor, veja a [documentação para contribuições](CONTRIBUTING_pt-BR.md) e siga os modelos em markdown para issues, relatórios de bugs e PR.

Verifique o Discord do projeto, com os proprietários do projeto, ou através de issues/PRs existentes para evitar trabalho duplicado.
Por favor, certifique-se de marcar todos os itens acima com os identificadores de projeto relevantes ou sua contribuição poderá se perder.
Exemplos de tags: `backend`, `bindings`, `python-bindings`, `documentation`, etc.

## Citação

Se você utilizar este repositório, modelos ou dados em um projeto downstream, considere citá-lo com:
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
