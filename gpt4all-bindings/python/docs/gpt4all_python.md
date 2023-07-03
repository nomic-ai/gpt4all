# GPT4All Python API
The `GPT4All` python package provides bindings to our C/C++ model backend libraries.
The source code and local build instructions can be found [here](https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python).


## Quickstart

```bash
pip install gpt4all
```

=== "GPT4All Example"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All("orca-mini-3b.ggmlv3.q4_0.bin")
    output = model.generate("The capital of France is ", max_tokens=3)
    print(output)
    ```
=== "Output"
    ```
    1. Paris
    ```

### Chatting with GPT4All
Local LLMs can be optimized for chat conversions by reusing previous computational history.

Use the GPT4All `chat_session` context manager to hold chat conversations with the model.

=== "GPT4All Example"
    ``` py
    model = GPT4All(model_name='orca-mini-3b.ggmlv3.q4_0.bin')
    with model.chat_session():
        response = model.generate(prompt='hello', top_k=1)
        response = model.generate(prompt='write me a short poem', top_k=1)
        response = model.generate(prompt='thank you', top_k=1)
        print(model.current_chat_session)
    ```
=== "Output"
    ``` json
    [
       {
          'role': 'user',
          'content': 'hello'
       },
       {
          'role': 'assistant',
          'content': 'What is your name?'
       },
       {
          'role': 'user',
          'content': 'write me a short poem'
       },
       {
          'role': 'assistant',
          'content': "I would love to help you with that! Here's a short poem I came up with:\nBeneath the autumn leaves,\nThe wind whispers through the trees.\nA gentle breeze, so at ease,\nAs if it were born to play.\nAnd as the sun sets in the sky,\nThe world around us grows still."
       },
       {
          'role': 'user',
          'content': 'thank you'
       },
       {
          'role': 'assistant',
          'content': "You're welcome! I hope this poem was helpful or inspiring for you. Let me know if there is anything else I can assist you with."
       }
    ]
    ```
When using GPT4All models in the chat_session context:

- The model is given a prompt template which makes it chatty.
- Internal K/V caches are preserved from previous conversation history speeding up inference.


### Generation Parameters

::: gpt4all.gpt4all.GPT4All.generate


### Streaming Generations
To interact with GPT4All responses as the model generates, use the `streaming = True` flag during generation.

=== "GPT4All Streaming Example"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All("orca-mini-3b.ggmlv3.q4_0.bin")
    tokens = []
    for token in model.generate("The capital of France is", max_tokens=20, streaming=True):
        tokens.append(token)
    print(tokens)
    ```
=== "Output"
    ```
    [' Paris', ' is', ' a', ' city', ' that', ' has', ' been', ' a', ' major', ' cultural', ' and', ' economic', ' center', ' for', ' over', ' ', '2', ',', '0', '0']
    ```

#### Streaming and Chat Sessions
When streaming tokens in a chat session, you must manually handle collection and updating of the chat history.

```python
from gpt4all import GPT4All
model = GPT4All("orca-mini-3b.ggmlv3.q4_0.bin")

with model.chat_session():
    tokens = list(model.generate(prompt='hello', top_k=1, streaming=True))
    model.current_chat_session.append({'role': 'assistant', 'content': ''.join(tokens)})

    tokens = list(model.generate(prompt='write me a poem about dogs', top_k=1, streaming=True))
    model.current_chat_session.append({'role': 'assistant', 'content': ''.join(tokens)})

    print(model.current_chat_session)
```


::: gpt4all.gpt4all.GPT4All
