# GPT4All Chat UI

The [GPT4All Chat Client](https://gpt4all.io) lets you easily interact with any local large language model.

It is optimized to run 7-13B parameter LLMs on the CPU's of any computer running OSX/Windows/Linux.

## Running LLMs on CPU
The GPT4All Chat UI supports models from all newer versions of `llama.cpp` with `GGUF` models including the `Mistral`, `LLaMA2`, `LLaMA`, `OpenLLaMa`, `Falcon`, `MPT`, `Replit`, `Starcoder`, and `Bert` architectures

GPT4All maintains an official list of recommended models located in [models2.json](https://github.com/nomic-ai/gpt4all/blob/main/gpt4all-chat/metadata/models2.json). You can pull request new models to it and if accepted they will show up in the official download dialog.

#### Sideloading any GGUF model
If a model is compatible with the gpt4all-backend, you can sideload it into GPT4All Chat by:

1. Downloading your model in GGUF format. It should be a 3-8 GB file similar to the ones [here](https://huggingface.co/TheBloke/Orca-2-7B-GGUF/tree/main).
2. Identifying your GPT4All model downloads folder. This is the path listed at the bottom of the downloads dialog.
3. Placing your downloaded model inside GPT4All's model downloads folder.
4. Restarting your GPT4ALL app. Your model should appear in the model selection list.

## Plugins
GPT4All Chat Plugins allow you to expand the capabilities of Local LLMs.

### LocalDocs Plugin (Chat With Your Data)
LocalDocs is a GPT4All feature that allows you to chat with your local files and data.
It allows you to utilize powerful local LLMs to chat with private data without any data leaving your computer or server.
When using LocalDocs, your LLM will cite the sources that most likely contributed to a given output. Note, even an LLM equipped with LocalDocs can hallucinate. The LocalDocs plugin will utilize your documents to help answer prompts and you will see references appear below the response.

<p align="center">
  <img width="70%" src="https://github.com/nomic-ai/gpt4all/assets/10168/fe5dd3c0-b3cc-4701-98d3-0280dfbcf26f">
</p>

#### Enabling LocalDocs
1. Install the latest version of GPT4All Chat from [GPT4All Website](https://gpt4all.io).
2. Go to `Settings > LocalDocs tab`.
3. Download the SBert model
4. Configure a collection (folder) on your computer that contains the files your LLM should have access to. You can alter the contents of the folder/directory at anytime. As you
add more files to your collection, your LLM will dynamically be able to access them.
5. Spin up a chat session with any LLM (including external ones like ChatGPT but warning data will leave your machine!)
6. At the top right, click the database icon and select which collection you want your LLM to know about during your chat session.
7. You can begin searching with your localdocs even before the collection has completed indexing, but note the search will not include those parts of the collection yet to be indexed.

#### LocalDocs Capabilities
LocalDocs allows your LLM to have context about the contents of your documentation collection.

LocalDocs **can**:

- Query your documents based upon your prompt / question. Your documents will be searched for snippets that can be used to provide context for an answer. The most relevant snippets will be inserted into your prompts context, but it will be up to the underlying model to decide how best to use the provided context.

LocalDocs **cannot**:

- Answer general metadata queries (e.g. `What documents do you know about?`, `Tell me about my documents`)
- Summarize a single document (e.g. `Summarize my magna carta PDF.`)

See the Troubleshooting section for common issues.

#### How LocalDocs Works
LocalDocs works by maintaining an index of all data in the directory your collection is linked to. This index
consists of small chunks of each document that the LLM can receive as additional input when you ask it a question.
The general technique this plugin uses is called [Retrieval Augmented Generation](https://arxiv.org/abs/2005.11401).

These document chunks help your LLM respond to queries with knowledge about the contents of your data.
The number of chunks and the size of each chunk can be configured in the LocalDocs plugin settings tab.

LocalDocs supports the following file types:
```json
["txt", "doc", "docx", "pdf", "rtf", "odt", "html", "htm", "xls", "xlsx", "csv", "ods", "ppt", "pptx", "odp", "xml", "json", "log", "md", "org", "tex", "asc", "wks",
"wpd", "wps", "wri", "xhtml", "xht", "xslt", "yaml", "yml", "dtd", "sgml", "tsv", "strings", "resx",
"plist", "properties", "ini", "config", "bat", "sh", "ps1", "cmd", "awk", "sed", "vbs", "ics", "mht",
"mhtml", "epub", "djvu", "azw", "azw3", "mobi", "fb2", "prc", "lit", "lrf", "tcr", "pdb", "oxps",
"xps", "pages", "numbers", "key", "keynote", "abw", "zabw", "123", "wk1", "wk3", "wk4", "wk5", "wq1",
"wq2", "xlw", "xlr", "dif", "slk", "sylk", "wb1", "wb2", "wb3", "qpw", "wdb", "wks", "wku", "wr1",
"wrk", "xlk", "xlt", "xltm", "xltx", "xlsm", "xla", "xlam", "xll", "xld", "xlv", "xlw", "xlc", "xlm",
"xlt", "xln"]
```

#### Troubleshooting and FAQ
*My LocalDocs plugin isn't using my documents*

- Make sure LocalDocs is enabled for your chat session (the DB icon on the top-right should have a border)
- If your document collection is large, wait 1-2 minutes for it to finish indexing.


#### LocalDocs Roadmap
- Customize model fine-tuned with retrieval in the loop.
- Plugin compatibility with chat client server mode.

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
