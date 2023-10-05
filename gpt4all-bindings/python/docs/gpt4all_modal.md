# GPT4All with Modal Labs

You can easily query any GPT4All model on [Modal Labs](https://modal.com/) infrastructure!
## Example

```python
import modal

def download_model():
    import gpt4all
    #you can use any model from https://gpt4all.io/models/models2.json
    return gpt4all.GPT4All("ggml-gpt4all-j-v1.3-groovy.bin")

image=modal.Image.debian_slim().pip_install("gpt4all").run_function(download_model)
stub = modal.Stub("gpt4all", image=image)
@stub.cls(keep_warm=1)
class GPT4All:
    def __enter__(self):
        print("Downloading model")
        self.gptj = download_model()
        print("Loaded model")

    @modal.method()
    def generate(self):
        messages = [{"role": "user", "content": "Name 3 colors"}]
        completion = self.gptj.chat_completion(messages)
        print(f"Completion: {completion}")

@stub.local_entrypoint()
def main():
    model = GPT4All()
    for i in range(10):
        model.generate.call()
```
