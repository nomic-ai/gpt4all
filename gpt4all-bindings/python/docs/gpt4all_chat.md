# GPT4All Chat Client

The [GPT4All Chat Client](https://gpt4all.io) lets you easily interact with any local large language model.

It is optimized to run 7-13B parameter LLMs on the CPU's of any computer in computer running OSX/Windows/Linux.




## GPT4All Chat Server Mode

GPT4All Chat comes with a built-in server mode allowing you to programmatically interact
with any supported local LLM through a *very familiar* HTTP API. You can find the API documentation [here](https://platform.openai.com/docs/api-reference/completions).

Enabling server mode in the chat client will spin-up on an HTTP server running on `localhost` port
`4891` (the reverse of 1984).

You can begin using local LLMs in your AI powered apps by changing a single line of code: the bath path for requests.

```python
import os
import openai

openai.api_base = "http://localhost:4891/v1" 
#openai.api_base = "https://api.openai.com/v1"

openai.api_key = "not needed for a local LLM"

# Set up the prompt and other parameters for the API request
prompt = "Who is Michael Jordan?"

# model = "gpt-3.5-turbo"
#model = "mpt-7b-chat"
model = "gpt4all-j-v1.3-groovy"

# Make the API request
response = openai.Completion.create(
    model=model,
    prompt=prompt,
    max_tokens=50,
    temperature=0.28,
    top_p=0.95,
    n=1,
    echo=True,
    stream=False
)

# Print the generated completion
print(response)
```
