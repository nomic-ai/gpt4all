# Settings

## Application Settings

!!! note "General Application Settings"

    | Setting | Description | Default Value |
    | --- | --- | --- |
    | **Theme** | Color theme for the application. Options are `Light`, `Dark`, and `LegacyDark` | `Light` |
    | **Font Size** | Font size setting for text throughout the application. Options are Small, Medium, and Large | Small |
    | **Device** | Device that will run your models. Options are `Auto` (GPT4All chooses), `Metal` (Apple Silicon M1+), `CPU`, and `GPU` | `Auto` |
    | **Default Model** | Choose your preferred LLM to load by default on startup| Auto |
    | **Download Path** | Select a destination on your device to save downloaded models | Windows: `C:\Users\{username}\AppData\Local\nomic.ai\GPT4All`<br><br>Mac: `/Users/{username}/Library/Application Support/nomic.ai/GPT4All/`<br><br>Linux: `/home/{username}/.local/share/nomic.ai/GPT4All` |
    | **Enable Datalake** | Opt-in to sharing interactions with GPT4All community (**anonymous** and **optional**) | Off |

!!! note "Advanced Application Settings"

    | Setting | Description | Default Value |
    | --- | --- | --- |
    | **CPU Threads** | Number of concurrently running CPU threads (more can speed up responses) | 4 |
    | **Save Chat Context** | Save chat context to disk to pick up exactly where a model left off. | Off |
    | **Enable Local Server** | Allow any application on your device to use GPT4All via an OpenAI-compatible GPT4All API | Off |
    | **API Server Port** | Local HTTP port for the local API server | 4891 |

## Model Settings

!!! note "Model / Character Settings"

    | Setting | Description | Default Value |
    | --- | --- | --- |
    | **Name** | Unique name of this model / character| set by model uploader |
    | **System Prompt** | General instructions for the chats this model will be used for | set by model uploader |
    | **Prompt Template** | Format of user <-> assistant interactions for the chats this model will be used for | set by model uploader |

### Clone

You can **clone** an existing model, which allows you to save a configuration of a model file with different prompt templates and sampling settings.

### Sampling Settings

!!! note "Model Sampling Settings"

    | Setting             | Description                          | Default Value |
    |----------------------------|------------------------------------------|-----------|
    | **Context Length**         | Maximum length of input sequence in tokens        | 2048      |
    | **Max Length**             | Maximum length of response in tokens     | 4096      |
    | **Prompt Batch Size**      | Token batch size for parallel processing | 128      |
    | **Temperature**            | Lower temperature gives more likely generations | 0.7       |
    | **Top P**                  | Prevents choosing highly unlikely tokens  | 0.4       |
    | **Top K**                  | Size of selection pool for tokens         | 40        |
    | **Min P**                  | Minimum relative probability              | 0         |
    | **Repeat Penalty Tokens**  | Length to apply penalty                   | 64        |
    | **Repeat Penalty**         | Penalize repetitiveness                   | 1.18      |
    | **GPU Layers**             | How many model layers to load into VRAM     | 32        |

## LocalDocs Settings

!!! note "General LocalDocs Settings"

    | Setting | Description | Default Value |
    | --- | --- | --- |
    | **Allowed File Extensions** | Choose which file types will be indexed into LocalDocs collections as text snippets with embedding vectors | `.txt`, `.pdf`, `.md`, `.rst` |
    | **Use Nomic Embed API** | Use Nomic API to create LocalDocs collections fast and off-device; [Nomic API Key](https://atlas.nomic.ai/) required | Off |
    | **Embeddings Device** | Device that will run embedding models. Options are `Auto` (GPT4All chooses), `Metal` (Apple Silicon M1+), `CPU`, and `GPU` | `Auto` |
    | **Show Sources** | Titles of source files retrieved by LocalDocs will be displayed directly in your chats.| On |

!!! note "Advanced LocalDocs Settings"

    Note that increasing these settings can increase the likelihood of factual responses, but may result in slower generation times.

    | Setting | Description | Default Value |
    | --- | --- | --- |
    | **Document Snippet Size** | Number of string characters per document snippet | 512 |
    | **Maximum Document Snippets Per Prompt** | Upper limit for the number of snippets from your files LocalDocs can retrieve for LLM context | 3 |
