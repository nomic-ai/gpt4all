# GPT4All API Server

GPT4All provides a local API server that allows you to run LLMs over an HTTP API. 

## Key Features

- **Local Execution**: Run models on your own hardware for privacy and offline use.
- **LocalDocs Integration**: Run the API with relevant text snippets provided to your LLM from a [LocalDocs collections](../gpt4all_desktop/localdocs.md).
- **OpenAI API Compatibility**: Use existing OpenAI-compatible clients and tools with your local models.

## Activating the API Server

1. Open the GPT4All Chat Desktop Application.
2. Go to `Settings` > `Application` and scroll down to `Advanced`.
3. Check the box for the `"Enable Local API Server"` setting.
4. The server listens on port 4891 by default. You can choose another port number in the `"API Server Port"` setting.

## Connecting to the API Server

The base URL used for the API server is `http://localhost:4891/v1` (or `http://localhost:<PORT_NUM>/v1` if you are using a different port number). The server only accepts HTTP connections (not HTTPS) and only listens on localhost (127.0.0.1) (e.g. not to the IPv6 localhost address `::1`.)

## Examples

!!! note "Example GPT4All API calls"

    === "cURL"

        ```bash
        curl -X POST http://localhost:4891/v1/chat/completions -d '{
        "model": "Phi-3 Mini Instruct",
        "messages": [{"role":"user","content":"Who is Michael Jordan?"}],
        "max_tokens": 50,
        "temperature": 0.28,
        "top_p": 0.95,
        }'
        ```

    === "PowerShell"

        ```powershell
        Invoke-WebRequest -URI http://localhost:4891/v1/chat/completions -Method POST -ContentType application/json -Body '{
        "model": "Phi-3 Mini Instruct",
        "messages": [{"role":"user","content":"Who is Michael Jordan?"}],
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

Note: LocalDocs cannot be activated or selected through the API itself.

