# GPT4All Chat UI

The [GPT4All Chat Client](https://gpt4all.io) lets you easily interact with any local large language model.

It is optimized to run 7-13B parameter LLMs on the CPU's of any computer running OSX/Windows/Linux.


## Plugins
GPT4All Chat Plugins allow you to expand the capabilities of Local LLMs. All plugins are compatible with the
chat clients server mode.

### LocalDocs Plugin (Chat With Your Data)
LocalDocs is a GPT4All plugin that allows you to chat with your local files and data.
It allows you to utilize powerful local LLMs to chat with private data without any data leaving your computer or server.
When using LocalDocs, your LLM will cite the sources that most likely contributed to a given output. Note, even an LLM equipped with LocalDocs can hallucinate.

#### Enabling LocalDocs
1. Install the latest version of GPT4All Chat from https://gpt4all.io.
2. Go to `Settings > the LocalDocs tab`.
3. Configure a collection (folder) on your computer that contains the files your LLM should have access to. You can alter the contents of the folder/directory at anytime. As you
add more files to your collection, your LLM will dynamically be able to access them.
4. Spin up a chat session with any LLM (including external ones like ChatGPT but warning data will leave your machine!)
5. At the top right, click the database icon and select which collection you want your LLM to know about.
6. Start chatting! 

### How it works
LocalDocs works by maintaining an index of all data in the directory your collection is linked to. This index
consists of small chunks of each document that the LLM can receive as additional input when you ask it a question.
This helps it respond to your queries with knowledge about the contents of your data.
The number of chunks and the size of each chunk can be configured in the LocalDocs plugin settings tab.
For indexing speed purposes, LocalDocs uses pre-deep-learning n-gram and tfidf based retrieval when deciding
what documents your LLM should have as context in response to a question. You'll find its of comparable quality
with embedding based retrieval approaches but magnitudes faster to ingest data. Don't worry, embedding based semantic
search for retrieval is on the roadmap for those with more powerful computers - pick up the feature on Github!


## Server Mode

GPT4All Chat comes with a built-in server mode allowing you to programmatically interact
with any supported local LLM through a *very familiar* HTTP API. You can find the API documentation [here](https://platform.openai.com/docs/api-reference/completions).

Enabling server mode in the chat client will spin-up on an HTTP server running on `localhost` port
`4891` (the reverse of 1984). You can enable the webserver via `GPT4All Chat > Settings > Enable web server`.

Begin using local LLMs in your AI powered apps by changing a single line of code: the base path for requests.

```python
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

which gives the following response

```json
{
  "choices": [
    {
      "finish_reason": "stop",
      "index": 0,
      "logprobs": null,
      "text": "Who is Michael Jordan?\nMichael Jordan is a former professional basketball player who played for the Chicago Bulls in the NBA. He was born on December 30, 1963, and retired from playing basketball in 1998."
    }
  ],
  "created": 1684260896,
  "id": "foobarbaz",
  "model": "gpt4all-j-v1.3-groovy",
  "object": "text_completion",
  "usage": {
    "completion_tokens": 35,
    "prompt_tokens": 39,
    "total_tokens": 74
  }
}
```
