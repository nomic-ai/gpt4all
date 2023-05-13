# GPT4All with Python

In this package, we introduce Python bindings built around GPT4All's C/C++ model backends.

## Quickstart

```bash
pip install gpt4all
```

In Python, run the following commands to retrieve a GPT4All model and generate a response
to a prompt.

**Download Note*:**
By default, models are stored in `~/.cache/gpt4all/` (you can change this with `model_path`). If the file already exists, model download will be skipped.

```python
import gpt4all
gptj = gpt4all.GPT4All("ggml-gpt4all-j-v1.3-groovy")
messages = [{"role": "user", "content": "Name 3 colors"}]
gptj.chat_completion(messages)
```

## Give it a try!
[Google Colab Tutorial](https://colab.research.google.com/drive/1QRFHV5lj1Kb7_tGZZGZ-E6BfX6izpeMI?usp=sharing)


## Best Practices
GPT4All models are designed to run locally on your own CPU. Large prompts may require longer computation time and
result in worse performance. Giving an instruction to the model will typically produce the best results.

There are two methods to interface with the underlying language model, `chat_completion()` and `generate()`. Chat completion formats a user-provided message dictionary into a prompt template (see API documentation for more details and options). This will usually produce much better results and is the approach we recommend. You may also prompt the model with `generate()` which will just pass the raw input string to the model. 