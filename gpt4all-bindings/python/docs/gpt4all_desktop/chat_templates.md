## What are chat templates?
Natively, large language models only know how to complete plain text and do not know the difference between their input and their output. In order to support a chat with a person, LLMs are designed to use a template to convert the conversation to plain text using a specific format.

For a given model, it is important to use an appropriate chat template, as each model is designed to work best with a specific format. The chat templates included with the built-in models should be sufficient for most purposes.

There are two reasons you would want to alter the chat template:

- You are sideloading a model and there is no chat template available,
- You would like to have greater control over the input to the LLM than a system message provides.


## What is a system message?
A system message is a message that controls the responses from the LLM in a way that affects the entire conversation. System messages can be short, such as "Speak like a pirate.", or they can be long and contain a lot of context for the LLM to keep in mind.

Not all models are designed to use a system message, so they work with some models better than others.


## How do I customize the chat template or system message?
To customize the chat template or system message, go to Settings > Model. Make sure to select the correct model at the top. If you clone a model, you can use a different chat template or system message from the base model, enabling you to use different settings for each conversation.

These settings take effect immediately. After changing them, you can click "Redo last response" in the chat view, and the response will take the new settings into account.


## Do I need to write a chat template?
You typically do not need to write your own chat template. The exception is models that are not in the official model list and do not come with a chat template built-in. These will show a "Clear" option above the chat template field in the Model Settings page instead of a "Reset" option. See the section on [finding] or [creating] a chat template.

[finding]: #how-do-i-find-a-chat-template
[creating]: #advanced-how-do-chat-templates-work


## What changed in GPT4All v3.5?
GPT4All v3.5 overhauled the chat template system. There are three crucial differences:

- The chat template now formats an entire conversation instead of a single pair of messages,
- The chat template now uses Jinja syntax instead of `%1` and `%2` placeholders,
- And the system message should no longer contain control tokens or trailing whitespace.

If you are using any chat templates or system messages that had been added or altered from the default before upgrading to GPT4All v3.5 or newer, these will no longer work. See below for how to solve common errors you may see after upgrading.


## Error/Warning: System message is not plain text.
This is easy to fix. Go to the model's settings and look at the system prompt. There are three things to look for:

- Control tokens such as `<|im_start|>`, `<|start_header_id|>`, or `<|system|>`
- A prefix such as `### System` or `SYSTEM:`
- Trailing whitespace, such as a space character or blank line.

If you see any of these things, remove them. For example, this legacy system prompt:
```
<|start_header_id|>system<|end_header_id|>
You are a helpful assistant.<|eot_id|>
```

Should become this:
```
You are a helpful assistant.
```

If you do not see anything that needs to be changed, you can dismiss the error by making a minor modification to the message and then changing it back.

If you see a warning, your system message does not appear to be plain text. If you believe this warning is incorrect, it can be safely ignored. If in doubt, ask on the [Discord].

[Discord]: https://discord.gg/mGZE39AS3e


## Error: Legacy system prompt needs to be updated in Settings.
This is the same as [above][above-1], but appears on the chat page.

[above-1]: #errorwarning-system-message-is-not-plain-text


## Error/Warning: Chat template is not in Jinja format.
This is the result of attempting to use an old-style template (possibly from a previous version) in GPT4All 3.5+.

Go to the Model Settings page and select the affected model. If you see a "Reset" button, and you have not intentionally modified the prompt template, you can click "Reset". Otherwise, this is what you can do:

1. Back up your chat template by copying it safely to a text file and saving it. In the next step, it will be removed from GPT4All.
2. Click "Reset" or "Clear".
3. If you clicked "Clear", the chat template is now gone. Follow the steps to [find][finding] or [create][creating] a basic chat template for your model.
4. Customize the chat template to suit your needs. For help, read the section about [creating] a chat template.


## Error: Legacy prompt template needs to be updated in Settings.
This is the same as [above][above-2], but appears on the chat page.

[above-2]: #errorwarning-chat-template-is-not-in-jinja-format


## The chat template has a syntax error.
If there is a syntax error while editing the chat template, the details will be displayed in an error message above the input box. This could be because the chat template is not actually in Jinja format (see [above][above-2]).

Otherwise, you have either typed something correctly, or the model comes with a template that is incompatible with GPT4All. See [the below section][creating] on creating chat templates and make sure that everything is correct. When in doubt, ask on the [Discord].


## Error: No chat template configured.
This may appear for models that are not from the official model list and do not include a chat template. Older versions of GPT4All picked a poor default in this case. You will get much better results if you follow the steps to [find][finding] or [create][creating] a chat template for your model.


## Error: The chat template cannot be blank.
If the button above the chat template on the Model Settings page says "Clear", see [above][above-3]. If you see "Reset", click that button to restore a reasonable default. Also see the section on [syntax errors][chat-syntax-error].

[above-3]: #error-no-chat-template-configured
[chat-syntax-error]: #the-chat-template-has-a-syntax-error


## How do I find a chat template?
When in doubt, you can always ask the [Discord] community for help. Below are the instructions to find one on your own.

The authoritative source for a model's chat template is the HuggingFace repo that the original (non-GGUF) model came from. First, you should find this page. If you just have a model file, you can try a google search for the model's name. If you know the page you downloaded the GGUF model from, its README usually links to the original non-GGUF model.

Once you have located the original model, there are two methods you can use to extract its chat template. Pick whichever one you are most comfortable with.

### Using the CLI (all models)
1. Install `jq` using your preferred package manager - e.g. Chocolatey (Windows), Homebrew (macOS), or apt (Ubuntu).
2. Download `tokenizer_config.json` from the model's "Files and versions" tab.
3. Open a command prompt in the directory which you have downloaded the model file.
4. Run `jq -r ".chat_template" tokenizer_config.json`. This shows the chat template in a human-readable form. You can copy this and paste it into the settings page.
5. (Optional) You can save the output to a text file like this: `jq -r ".chat_template" tokenizer_config.json >chat_template.txt`

If the output is "null", the model does not provide a chat template. See the [below instructions][creating] on creating a chat template.

### Python (open models)
1. Install `transformers` using your preferred python package manager, e.g. `pip install transformers`. Make sure it is at least version v4.43.0.
2. Copy the ID of the HuggingFace model, using the clipboard icon next to the name. For example, if the URL is `https://huggingface.co/NousResearch/Hermes-2-Pro-Llama-3-8B`, the ID is `NousResearch/Hermes-2-Pro-Llama-3-8B`.
3. Open a python interpreter (`python`) and run the following commands. Change the model ID in the example to the one you copied.
```
>>> from transformers import AutoTokenizer
>>> tokenizer = AutoTokenizer.from_pretrained('NousResearch/Hermes-2-Pro-Llama-3-8B')
>>> print(tokenizer.get_chat_template())
```
You can copy the output and paste it into the settings page.
4. (Optional) You can save the output to a text file like this:
```
>>> open('chat_template.txt', 'w').write(tokenizer.get_chat_template())
```

If you get a ValueError exception, this model does not provide a chat template. See the [below instructions][creating] on creating a chat template.


### Python (gated models)
Some models, such as Llama and Mistral, do not allow public access to their chat template. You must either use the CLI method above, or follow the following instructions to use Python:

1. For these steps, you must have git and git-lfs installed.
2. You must have a HuggingFace account and be logged in.
3. You must already have access to the gated model. Otherwise, request access.
4. You must have an SSH key configured for git access to HuggingFace.
5. `git clone` the model's HuggingFace repo using the SSH clone URL. There is no need to download the entire model, which is very large. A good way to do this on Linux is:
```console
$ GIT_LFS_SKIP_SMUDGE=1 git clone hf.co:meta-llama/Llama-3.1-8B-Instruct.git
$ cd Llama-3.1-8B-Instruct
$ git lfs pull -I "tokenizer.*"
```
6. Follow the above instructions for open models, but replace the model ID with the path to the directory containing `tokenizer\_config.json`:
```
>>> tokenizer = AutoTokenizer.from_pretrained('.')
```


## Advanced: How do chat templates work?
The chat template is applied to the entire conversation you see in the chat window. The template loops over the list of messages, each containing `role` and `content` fields. `role` is either `user`, `assistant`, or `system`.

GPT4All also supports the special variables `bos_token`, `eos_token`, and `add_generation_prompt`. See the [HuggingFace docs] for what those do.

[HuggingFace docs]: https://huggingface.co/docs/transformers/v4.46.3/en/chat_templating#special-variables


## Advanced: How do I make a chat template?
The best way to create a chat template is to start by using an existing one as a reference. Then, modify it to use the format documented for the given model. Its README page may explicitly give an example of its template. Or, it may mention the name of a well-known standard template, such as ChatML, Alpaca, Vicuna. GPT4All does not yet include presets for these templates, so they will have to be found in other models or taken from the community.

For more information, see the very helpful [HuggingFace guide]. Some of this is not applicable, such as the information about tool calling and RAG - GPT4All implements those features differently.

Some models use a prompt template that does not intuitively map to a multi-turn chat, because it is more intended for single instructions. The [FastChat] implementation of these templates is a useful reference for the correct way to extend them to multiple messages.

[HuggingFace guide]: https://huggingface.co/docs/transformers/v4.46.3/en/chat_templating#advanced-template-writing-tips
[FastChat]: https://github.com/lm-sys/FastChat/blob/main/fastchat/conversation.py


# Advanced: What are GPT4All v1 templates?
GPT4All supports its own template syntax, which is nonstandard but provides complete control over the way LocalDocs sources and file attachments are inserted into the conversation. These templates begin with `{# gpt4all v1 #}` and look similar to the example below.

For standard templates, GPT4All combines the user message, sources, and attachments into the `content` field. For GPT4All v1 templates, this is not done, so they must be used directly in the template for those features to work correctly.

```jinja
{# gpt4all v1 #}
{%- for message in messages %}
    {{- '<|start_header_id|>' + message['role'] + '<|end_header_id|>\n\n' }}
    {%- if message['role'] == 'user' %}
        {%- for source in message['sources'] %}
            {%- if loop.first %}
                {{- '### Context:\n' }}
            {%- endif %}
            {{- 'Collection: ' + source['collection'] + '\n'   +
                'Path: '       + source['path']       + '\n'   +
                'Excerpt: '    + source['text']       + '\n\n' }}
        {%- endfor %}
    {%- endif %}
    {%- for attachment in message['prompt_attachments'] %}
        {{- attachment['processed_content'] + '\n\n' }}
    {%- endfor %}
    {{- message['content'] | trim }}
    {{- '<|eot_id|>' }}
{%- endfor %}
{%- if add_generation_prompt %}
    {{- '<|start_header_id|>assistant<|end_header_id|>\n\n' }}
{%- endif %}
```
