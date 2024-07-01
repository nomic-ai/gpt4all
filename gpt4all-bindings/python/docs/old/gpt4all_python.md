# GPT4All Python Generation API
The `GPT4All` python package provides bindings to our C/C++ model backend libraries.
The source code and local build instructions can be found [here](https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python).


## Quickstart
```bash
pip install gpt4all
```

``` py
from gpt4all import GPT4All
model = GPT4All("orca-mini-3b-gguf2-q4_0.gguf")
```

This will:

- Instantiate `GPT4All`,  which is the primary public API to your large language model (LLM).
- Automatically download the given model to `~/.cache/gpt4all/` if not already present.

Read further to see how to chat with this model.


### Chatting with GPT4All
To start chatting with a local LLM, you will need to start a chat session. Within a chat session, the model will be
prompted with the appropriate template, and history will be preserved between successive calls to `generate()`.

=== "GPT4All Example"
    ``` py
    model = GPT4All(model_name='orca-mini-3b-gguf2-q4_0.gguf')
    with model.chat_session():
        response1 = model.generate(prompt='hello', temp=0)
        response2 = model.generate(prompt='write me a short poem', temp=0)
        response3 = model.generate(prompt='thank you', temp=0)
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

When using GPT4All models in the `chat_session()` context:

- Consecutive chat exchanges are taken into account and not discarded until the session ends; as long as the model has capacity.
- A system prompt is inserted into the beginning of the model's context.
- Each prompt passed to `generate()` is wrapped in the appropriate prompt template. If you pass `allow_download=False`
  to GPT4All or are using a model that is not from the official models list, you must pass a prompt template using the
  `prompt_template` parameter of `chat_session()`.

NOTE: If you do not use `chat_session()`, calls to `generate()` will not be wrapped in a prompt template. This will
cause the model to *continue* the prompt instead of *answering* it. When in doubt, use a chat session, as many newer
models are designed to be used exclusively with a prompt template.

[models3.json]: https://github.com/nomic-ai/gpt4all/blob/main/gpt4all-chat/metadata/models3.json


### Streaming Generations
To interact with GPT4All responses as the model generates, use the `streaming=True` flag during generation.

=== "GPT4All Streaming Example"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All("orca-mini-3b-gguf2-q4_0.gguf")
    tokens = []
    with model.chat_session():
        for token in model.generate("What is the capital of France?", streaming=True):
            tokens.append(token)
    print(tokens)
    ```
=== "Output"
    ```
    [' The', ' capital', ' of', ' France', ' is', ' Paris', '.']
    ```


### The Generate Method API
::: gpt4all.gpt4all.GPT4All.generate


## Examples & Explanations
### Influencing Generation
The three most influential parameters in generation are _Temperature_ (`temp`), _Top-p_ (`top_p`) and _Top-K_ (`top_k`).
In a nutshell, during the process of selecting the next token, not just one or a few are considered, but every single
token in the vocabulary is given a probability. The parameters can change the field of candidate tokens.

- **Temperature** makes the process either more or less random. A _Temperature_ above 1 increasingly "levels the playing
  field", while at a _Temperature_ between 0 and 1 the likelihood of the best token candidates grows even more. A
  _Temperature_ of 0 results in selecting the best token, making the output deterministic. A _Temperature_ of 1
  represents a neutral setting with regard to randomness in the process.

- _Top-p_ and _Top-K_ both narrow the field:
    - **Top-K** limits candidate tokens to a fixed number after sorting by probability. Setting it higher than the
      vocabulary size deactivates this limit.
    - **Top-p** selects tokens based on their total probabilities. For example, a value of 0.8 means "include the best
      tokens, whose accumulated probabilities reach or just surpass 80%". Setting _Top-p_ to 1, which is 100%,
      effectively disables it.

The recommendation is to keep at least one of _Top-K_ and _Top-p_ active. Other parameters can also influence
generation; be sure to review all their descriptions.


### Specifying the Model Folder
The model folder can be set with the `model_path` parameter when creating a `GPT4All` instance. The example below is
is the same as if it weren't provided; that is, `~/.cache/gpt4all/` is the default folder.

``` py
from pathlib import Path
from gpt4all import GPT4All
model = GPT4All(model_name='orca-mini-3b-gguf2-q4_0.gguf', model_path=Path.home() / '.cache' / 'gpt4all')
```

If you want to point it at the chat GUI's default folder, it should be:
=== "macOS"
    ``` py
    from pathlib import Path
    from gpt4all import GPT4All

    model_name = 'orca-mini-3b-gguf2-q4_0.gguf'
    model_path = Path.home() / 'Library' / 'Application Support' / 'nomic.ai' / 'GPT4All'
    model = GPT4All(model_name, model_path)
    ```
=== "Windows"
    ``` py
    from pathlib import Path
    from gpt4all import GPT4All
    import os
    model_name = 'orca-mini-3b-gguf2-q4_0.gguf'
    model_path = Path(os.environ['LOCALAPPDATA']) / 'nomic.ai' / 'GPT4All'
    model = GPT4All(model_name, model_path)
    ```
=== "Linux"
    ``` py
    from pathlib import Path
    from gpt4all import GPT4All

    model_name = 'orca-mini-3b-gguf2-q4_0.gguf'
    model_path = Path.home() / '.local' / 'share' / 'nomic.ai' / 'GPT4All'
    model = GPT4All(model_name, model_path)
    ```

Alternatively, you could also change the module's default model directory:

``` py
from pathlib import Path
from gpt4all import GPT4All, gpt4all
gpt4all.DEFAULT_MODEL_DIRECTORY = Path.home() / 'my' / 'models-directory'
model = GPT4All('orca-mini-3b-gguf2-q4_0.gguf')
```


### Managing Templates
When using a `chat_session()`, you may customize the system prompt, and set the prompt template if necessary:

=== "GPT4All Custom Session Templates Example"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All('wizardlm-13b-v1.2.Q4_0.gguf')
    system_template = 'A chat between a curious user and an artificial intelligence assistant.\n'
    # many models use triple hash '###' for keywords, Vicunas are simpler:
    prompt_template = 'USER: {0}\nASSISTANT: '
    with model.chat_session(system_template, prompt_template):
        response1 = model.generate('why is the grass green?')
        print(response1)
        print()
        response2 = model.generate('why is the sky blue?')
        print(response2)
    ```
=== "Possible Output"
    ```
    The color of grass can be attributed to its chlorophyll content, which allows it
    to absorb light energy from sunlight through photosynthesis. Chlorophyll absorbs
    blue and red wavelengths of light while reflecting other colors such as yellow
    and green. This is why the leaves appear green to our eyes.

    The color of the sky appears blue due to a phenomenon called Rayleigh scattering,
    which occurs when sunlight enters Earth's atmosphere and interacts with air
    molecules such as nitrogen and oxygen. Blue light has shorter wavelength than
    other colors in the visible spectrum, so it is scattered more easily by these
    particles, making the sky appear blue to our eyes.
    ```


### Without Online Connectivity
To prevent GPT4All from accessing online resources, instantiate it with `allow_download=False`. When using this flag,
there will be no default system prompt by default, and you must specify the prompt template yourself.

You can retrieve a model's default system prompt and prompt template with an online instance of GPT4All:

=== "Prompt Template Retrieval"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All('orca-mini-3b-gguf2-q4_0.gguf')
    print(repr(model.config['systemPrompt']))
    print(repr(model.config['promptTemplate']))
    ```
=== "Output"
    ```py
    '### System:\nYou are an AI assistant that follows instruction extremely well. Help as much as you can.\n\n'
    '### User:\n{0}\n### Response:\n'
    ```

Then you can pass them explicitly when creating an offline instance:

``` py
from gpt4all import GPT4All
model = GPT4All('orca-mini-3b-gguf2-q4_0.gguf', allow_download=False)

system_prompt = '### System:\nYou are an AI assistant that follows instruction extremely well. Help as much as you can.\n\n'
prompt_template = '### User:\n{0}\n\n### Response:\n'

with model.chat_session(system_prompt=system_prompt, prompt_template=prompt_template):
    ...
```

### Interrupting Generation
The simplest way to stop generation is to set a fixed upper limit with the `max_tokens` parameter.

If you know exactly when a model should stop responding, you can add a custom callback, like so:

=== "GPT4All Custom Stop Callback"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All('orca-mini-3b-gguf2-q4_0.gguf')

    def stop_on_token_callback(token_id, token_string):
        # one sentence is enough:
        if '.' in token_string:
            return False
        else:
            return True

    response = model.generate('Blue Whales are the biggest animal to ever inhabit the Earth.',
                              temp=0, callback=stop_on_token_callback)
    print(response)
    ```
=== "Output"
    ```
     They can grow up to 100 feet (30 meters) long and weigh as much as 20 tons (18 metric tons).
    ```


## API Documentation
::: gpt4all.gpt4all.GPT4All
