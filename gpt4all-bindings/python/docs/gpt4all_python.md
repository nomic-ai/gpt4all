# GPT4All Python Generation API
The `GPT4All` python package provides bindings to our C/C++ model backend libraries.
The source code and local build instructions can be found [here](https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python).


## Quickstart
```bash
pip install gpt4all
```

=== "GPT4All Example"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All("orca-mini-3b-gguf2-q4_0.gguf")
    output = model.generate("The capital of France is ", max_tokens=3)
    print(output)
    ```
=== "Output"
    ```
    1. Paris
    ```

This will:

- Instantiate `GPT4All`,  which is the primary public API to your large language model (LLM).
- Automatically download the given model to `~/.cache/gpt4all/` if not already present.
- Through `model.generate(...)` the model starts working on a response. There are various ways to
  steer that process. Here, `max_tokens` sets an upper limit, i.e. a hard cut-off point to the output.


### Chatting with GPT4All
Local LLMs can be optimized for chat conversations by reusing previous computational history.

Use the GPT4All `chat_session` context manager to hold chat conversations with the model.

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

When using GPT4All models in the `chat_session` context:

- Consecutive chat exchanges are taken into account and not discarded until the session ends; as long as the model has capacity.
- Internal K/V caches are preserved from previous conversation history, speeding up inference.
- The model is given a system and prompt template which make it chatty. Depending on `allow_download=True` (default),
  it will obtain the latest version of [models2.json] from the repository, which contains specifically tailored templates
  for models. Conversely, if it is not allowed to download, it falls back to default templates instead.

[models2.json]: https://github.com/nomic-ai/gpt4all/blob/main/gpt4all-chat/metadata/models2.json


### Streaming Generations
To interact with GPT4All responses as the model generates, use the `streaming=True` flag during generation.

=== "GPT4All Streaming Example"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All("orca-mini-3b-gguf2-q4_0.gguf")
    tokens = []
    for token in model.generate("The capital of France is", max_tokens=20, streaming=True):
        tokens.append(token)
    print(tokens)
    ```
=== "Output"
    ```
    [' Paris', ' is', ' a', ' city', ' that', ' has', ' been', ' a', ' major', ' cultural', ' and', ' economic', ' center', ' for', ' over', ' ', '2', ',', '0', '0']
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

=== "GPT4All Model Folder Example"
    ``` py
    from pathlib import Path
    from gpt4all import GPT4All
    model = GPT4All(model_name='orca-mini-3b-gguf2-q4_0.gguf',
                    model_path=(Path.home() / '.cache' / 'gpt4all'),
                    allow_download=False)
    response = model.generate('my favorite 3 fruits are:', temp=0)
    print(response)
    ```
=== "Output"
    ```
    My favorite three fruits are apples, bananas and oranges.
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
import gpt4all.gpt4all
gpt4all.gpt4all.DEFAULT_MODEL_DIRECTORY = Path.home() / 'my' / 'models-directory'
from gpt4all import GPT4All
model = GPT4All('orca-mini-3b-gguf2-q4_0.gguf')
...
```


### Managing Templates
Session templates can be customized when starting a `chat_session` context:

=== "GPT4All Custom Session Templates Example"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All('wizardlm-13b-v1.2.Q4_0.gguf')
    system_template = 'A chat between a curious user and an artificial intelligence assistant.'
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

To do the same outside a session, the input has to be formatted manually. For example:

=== "GPT4All Templates Outside a Session Example"
    ``` py
    model = GPT4All('wizardlm-13b-v1.2.Q4_0.gguf')
    system_template = 'A chat between a curious user and an artificial intelligence assistant.'
    prompt_template = 'USER: {0}\nASSISTANT: '
    prompts = ['name 3 colors', 'now name 3 fruits', 'what were the 3 colors in your earlier response?']
    first_input = system_template + prompt_template.format(prompts[0])
    response = model.generate(first_input, temp=0)
    print(response)
    for prompt in prompts[1:]:
        response = model.generate(prompt_template.format(prompt), temp=0)
        print(response)
    ```
=== "Output"
    ```
    1) Red
    2) Blue
    3) Green

    1. Apple
    2. Banana
    3. Orange

    The colors in my previous response are blue, green and red.
    ```

Ultimately, the method `GPT4All._format_chat_prompt_template()` is responsible for formatting templates. It can be
customized in a subclass. As an example:

=== "Custom Subclass"
    ``` py
    from itertools import cycle
    from gpt4all import GPT4All

    class RotatingTemplateGPT4All(GPT4All):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self._templates = [
                "Respond like a pirate.",
                "Respond like a politician.",
                "Respond like a philosopher.",
                "Respond like a Klingon.",
            ]
            self._cycling_templates = cycle(self._templates)

        def _format_chat_prompt_template(
            self,
            messages: list,
            default_prompt_header: str = "",
            default_prompt_footer: str = "",
        ) -> str:
            full_prompt = default_prompt_header + "\n\n" if default_prompt_header != "" else ""
            for message in messages:
                if message["role"] == "user":
                    user_message = f"USER: {message['content']} {next(self._cycling_templates)}\n"
                    full_prompt += user_message
                if message["role"] == "assistant":
                    assistant_message = f"ASSISTANT: {message['content']}\n"
                    full_prompt += assistant_message
            full_prompt += "\n\n" + default_prompt_footer if default_prompt_footer != "" else ""
            print(full_prompt)
            return full_prompt
    ```
=== "GPT4All Custom Subclass Example"
    ``` py
    model = RotatingTemplateGPT4All('wizardlm-13b-v1.2.Q4_0.gguf')
    with model.chat_session():  # starting a session is optional in this example
        response1 = model.generate("hi, who are you?")
        print(response1)
        print()
        response2 = model.generate("what can you tell me about snakes?")
        print(response2)
        print()
        response3 = model.generate("what's your opinion on Chess?")
        print(response3)
        print()
        response4 = model.generate("tell me about ancient Rome.")
        print(response4)
    ```
=== "Possible Output"
    ```
    USER: hi, who are you? Respond like a pirate.

    Pirate: Ahoy there mateys! I be Cap'n Jack Sparrow of the Black Pearl.

    USER: what can you tell me about snakes? Respond like a politician.

    Politician: Snakes have been making headlines lately due to their ability to
    slither into tight spaces and evade capture, much like myself during my last
    election campaign. However, I believe that with proper education and
    understanding of these creatures, we can work together towards creating a
    safer environment for both humans and snakes alike.

    USER: what's your opinion on Chess? Respond like a philosopher.

    Philosopher: The game of chess is often used as an analogy to illustrate the
    complexities of life and decision-making processes. However, I believe that it
    can also be seen as a reflection of our own consciousness and subconscious mind.
    Just as each piece on the board has its unique role to play in shaping the
    outcome of the game, we too have different roles to fulfill in creating our own
    personal narrative.

    USER: tell me about ancient Rome. Respond like a Klingon.

    Klingon: Ancient Rome was once a great empire that ruled over much of Europe and
    the Mediterranean region. However, just as the Empire fell due to internal strife
    and external threats, so too did my own house come crashing down when I failed to
    protect our homeworld from invading forces.
    ```


### Introspection
A less apparent feature is the capacity to log the final prompt that gets sent to the model. It relies on
[Python's logging facilities][py-logging] implemented in the `pyllmodel` module at the `INFO` level. You can activate it
for example with a `basicConfig`, which displays it on the standard error stream. It's worth mentioning that Python's
logging infrastructure offers [many more customization options][py-logging-cookbook].

[py-logging]: https://docs.python.org/3/howto/logging.html
[py-logging-cookbook]: https://docs.python.org/3/howto/logging-cookbook.html

=== "GPT4All Prompt Logging Example"
    ``` py
    import logging
    from gpt4all import GPT4All
    logging.basicConfig(level=logging.INFO)
    model = GPT4All('nous-hermes-llama2-13b.Q4_0.gguf')
    with model.chat_session('You are a geography expert.\nBe terse.',
                            '### Instruction:\n{0}\n### Response:\n'):
        response = model.generate('who are you?', temp=0)
        print(response)
        response = model.generate('what are your favorite 3 mountains?', temp=0)
        print(response)
    ```
=== "Output"
    ```
    INFO:gpt4all.pyllmodel:LLModel.prompt_model -- prompt:
    You are a geography expert.
    Be terse.

    ### Instruction:
    who are you?
    ### Response:

    ===/LLModel.prompt_model -- prompt/===
    I am an AI-powered chatbot designed to assist users with their queries related to geographical information.
    INFO:gpt4all.pyllmodel:LLModel.prompt_model -- prompt:
    ### Instruction:
    what are your favorite 3 mountains?
    ### Response:

    ===/LLModel.prompt_model -- prompt/===
    1) Mount Everest - Located in the Himalayas, it is the highest mountain on Earth and a significant challenge for mountaineers.
    2) Kangchenjunga - This mountain is located in the Himalayas and is the third-highest peak in the world after Mount Everest and K2.
    3) Lhotse - Located in the Himalayas, it is the fourth highest mountain on Earth and offers a challenging climb for experienced mountaineers.
    ```


### Without Online Connectivity
To prevent GPT4All from accessing online resources, instantiate it with `allow_download=False`. This will disable both
downloading missing models and [models2.json], which contains information about them. As a result, predefined templates
are used instead of model-specific system and prompt templates:

=== "GPT4All Default Templates Example"
    ``` py
    from gpt4all import GPT4All
    model = GPT4All('ggml-mpt-7b-chat.bin', allow_download=False)
    # when downloads are disabled, it will use the default templates:
    print("default system template:", repr(model.config['systemPrompt']))
    print("default prompt template:", repr(model.config['promptTemplate']))
    print()
    # even when inside a session:
    with model.chat_session():
        assert model.current_chat_session[0]['role'] == 'system'
        print("session system template:", repr(model.current_chat_session[0]['content']))
        print("session prompt template:", repr(model._current_prompt_template))
    ```
=== "Output"
    ```
    default system template: ''
    default prompt template: '### Human: \n{0}\n### Assistant:\n'

    session system template: ''
    session prompt template: '### Human: \n{0}\n### Assistant:\n'
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
