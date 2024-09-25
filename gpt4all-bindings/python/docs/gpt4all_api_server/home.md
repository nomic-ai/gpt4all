# GPT4All API Server

GPT4All provides a local API server that allows you to run LLMs over an HTTP API. 

## Key Features

- **Local Execution**: Run models on your own hardware for privacy and offline use.
- **LocalDocs Integration**: Run the API with relevant text snippets provided to your LLM from a [LocalDocs collection](../gpt4all_desktop/localdocs.md).
- **OpenAI API Compatibility**: Use existing OpenAI-compatible clients and tools with your local models.

## Activating the API Server

1. Open the GPT4All Chat Desktop Application.
2. Go to `Settings` > `Application` and scroll down to `Advanced`.
3. Check the box for the `"Enable Local API Server"` setting.
4. The server listens on port 4891 by default. You can choose another port number in the `"API Server Port"` setting.

## Connecting to the API Server

The base URL used for the API server is `http://localhost:4891/v1` (or `http://localhost:<PORT_NUM>/v1` if you are using a different port number). 

The server only accepts HTTP connections (not HTTPS) and only listens on localhost (127.0.0.1) (e.g. not to the IPv6 localhost address `::1`.)

## Examples

!!! note "Example GPT4All API calls"

    === "cURL"

        ```bash
        curl -X POST http://localhost:4891/v1/chat/completions -d '{
        "model": "Phi-3 Mini Instruct",
        "messages": [{"role":"user","content":"Who is Lionel Messi?"}],
        "max_tokens": 50,
        "temperature": 0.28
        }'
        ```

    === "PowerShell"

        ```powershell
        Invoke-WebRequest -URI http://localhost:4891/v1/chat/completions -Method POST -ContentType application/json -Body '{
        "model": "Phi-3 Mini Instruct",
        "messages": [{"role":"user","content":"Who is Lionel Messi?"}],
        "max_tokens": 50,
        "temperature": 0.28
        }'
        ```

## API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/v1/models` | List available models |
| GET | `/v1/models/<name>` | Get details of a specific model |
| POST | `/v1/completions` | Generate text completions |
| POST | `/v1/chat/completions` | Generate chat completions |

## LocalDocs Integration

You can use LocalDocs with the API server:

1. Open the Chats view in the GPT4All application.
2. Scroll to the bottom of the chat history sidebar.
3. Select the server chat (it has a different background color).
4. Activate LocalDocs collections in the right sidebar.

(Note: LocalDocs can currently only be activated through the GPT4All UI, not via the API itself).

Now, your API calls to your local LLM will have relevant references from your LocalDocs collection retrieved and placed in the input message for the LLM to respond to.

The references retrieved for your API call can be accessed in the API response object at 

`response["choices"][0]["references"]`

The data included in the `references` are:

- `text`: the actual text content from the snippet that was extracted from the reference document

- `author`: the author of the reference document (if available)

- `date`: the date of creation of the reference document (if available)

- `page`: the page number the snippet is from (only available for PDF documents for now)

- `title`: the title of the reference document (if available)
