# GPT4All

In this package, we introduce Python bindings built around GPT4All's C/C++ ecosystem.

## Quickstart

```bash
pip install gpt4all
```

In Python, run the following commands to retrieve a GPT4All model and generate a response
to a prompt.

**Download Note*:*
By default, models are stored in `~/.cache/gpt4all/` (you can change this with `model_path`). If the file already exists, model download will be skipped.

```python
import gpt4all
gptj = gpt4all.GPT4All("ggml-gpt4all-j-v1.3-groovy")
messages = [{"role": "user", "content": "Name 3 colors"}]
gptj.chat_completion(messages)
```