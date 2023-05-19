# GPT4All Python API
The `GPT4All` package provides Python bindings and an API to our C/C++ model backend libraries.
The source code, README, and local build instructions can be found [here](https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python).


## Quickstart

```bash
pip install gpt4all
```

In Python, run the following commands to retrieve a GPT4All model and generate a response
to a prompt.

**Download Note:**
By default, models are stored in `~/.cache/gpt4all/` (you can change this with `model_path`). If the file already exists, model download will be skipped.

```python
import gpt4all
gptj = gpt4all.GPT4All("ggml-gpt4all-j-v1.3-groovy")
messages = [{"role": "user", "content": "Name 3 colors"}]
gptj.chat_completion(messages)
```

## Give it a try!
[Google Colab Tutorial](https://colab.research.google.com/drive/1QRFHV5lj1Kb7_tGZZGZ-E6BfX6izpeMI?usp=sharing)

## Supported Models
Python bindings support the following ggml architectures: `gptj`, `llama`, `mpt`. See API reference for more details.

## Best Practices

There are two methods to interface with the underlying language model, `chat_completion()` and `generate()`. Chat completion formats a user-provided message dictionary into a prompt template (see API documentation for more details and options). This will usually produce much better results and is the approach we recommend. You may also prompt the model with `generate()` which will just pass the raw input string to the model. 

## API Reference

::: gpt4all.gpt4all.GPT4All
