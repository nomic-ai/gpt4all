# GPT4All Node.js API

Native Node.js LLM bindings for all.

```sh
yarn add gpt4all@latest

npm install gpt4all@latest

pnpm install gpt4all@latest

```

## Contents

*   See [API Reference](#api-reference)
*   See [Examples](#api-example)
*   See [Developing](#develop)
*   GPT4ALL nodejs bindings created by [jacoobes](https://github.com/jacoobes), [limez](https://github.com/iimez) and the [nomic ai community](https://home.nomic.ai), for all to use.

## Api Example

### Chat Completion

```js
import { LLModel, createCompletion, DEFAULT_DIRECTORY, DEFAULT_LIBRARIES_DIRECTORY, loadModel } from '../src/gpt4all.js'

const model = await loadModel( 'mistral-7b-openorca.gguf2.Q4_0.gguf', { verbose: true, device: 'gpu' });

const completion1 = await createCompletion(model, 'What is 1 + 1?', { verbose: true, })
console.log(completion1.message)

const completion2 = await createCompletion(model, 'And if we add two?', {  verbose: true  })
console.log(completion2.message)

model.dispose()
```

### Embedding

```js
import { loadModel, createEmbedding } from '../src/gpt4all.js'

const embedder = await loadModel("all-MiniLM-L6-v2-f16.gguf", { verbose: true, type: 'embedding'})

console.log(createEmbedding(embedder, "Maybe Minecraft was the friends we made along the way"));
```

### Chat Sessions

```js
import { loadModel, createCompletion } from "../src/gpt4all.js";

const model = await loadModel("orca-mini-3b-gguf2-q4_0.gguf", {
    verbose: true,
    device: "gpu",
});

const chat = await model.createChatSession();

await createCompletion(
    chat,
    "Why are bananas rather blue than bread at night sometimes?",
    {
        verbose: true,
    }
);
await createCompletion(chat, "Are you sure?", { verbose: true, });

```

### Streaming responses

```js
import gpt from "../src/gpt4all.js";

const model = await gpt.loadModel("mistral-7b-openorca.gguf2.Q4_0.gguf", {
    device: "gpu",
});

process.stdout.write("### Stream:");
const stream = gpt.createCompletionStream(model, "How are you?");
stream.tokens.on("data", (data) => {
    process.stdout.write(data);
});
//wait till stream finishes. We cannot continue until this one is done.
await stream.result;
process.stdout.write("\n");

process.stdout.write("### Stream with pipe:");
const stream2 = gpt.createCompletionStream(
    model,
    "Please say something nice about node streams."
);
stream2.tokens.pipe(process.stdout);
await stream2.result;
process.stdout.write("\n");

console.log("done");
model.dispose();
```

### Async Generators

```js
import gpt from "../src/gpt4all.js";

const model = await gpt.loadModel("mistral-7b-openorca.gguf2.Q4_0.gguf", {
    device: "gpu",
});

process.stdout.write("### Generator:");
const gen = gpt.createCompletionGenerator(model, "Redstone in Minecraft is Turing Complete. Let that sink in. (let it in!)");
for await (const chunk of gen) {
    process.stdout.write(chunk);
}

process.stdout.write("\n");
model.dispose();
```

## Develop

### Build Instructions

*   binding.gyp is compile config
*   Tested on Ubuntu. Everything seems to work fine
*   Tested on Windows. Everything works fine.
*   Sparse testing on mac os.
*   MingW works as well to build the gpt4all-backend. **HOWEVER**, this package works only with MSVC built dlls.

### Requirements

*   git
*   [node.js >= 18.0.0](https://nodejs.org/en)
*   [yarn](https://yarnpkg.com/)
*   [node-gyp](https://github.com/nodejs/node-gyp)
    *   all of its requirements.
*   (unix) gcc version 12
*   (win) msvc version 143
    *   Can be obtained with visual studio 2022 build tools
*   python 3
*   On Windows and Linux, building GPT4All requires the complete Vulkan SDK. You may download it from here: https://vulkan.lunarg.com/sdk/home
*   macOS users do not need Vulkan, as GPT4All will use Metal instead.

### Build (from source)

```sh
git clone https://github.com/nomic-ai/gpt4all.git
cd gpt4all-bindings/typescript
```

*   The below shell commands assume the current working directory is `typescript`.

*   To Build and Rebuild:

```sh
yarn
```

*   llama.cpp git submodule for gpt4all can be possibly absent. If this is the case, make sure to run in llama.cpp parent directory

```sh
git submodule update --init --depth 1 --recursive
```

```sh
yarn build:backend
```

This will build platform-dependent dynamic libraries, and will be located in runtimes/(platform)/native The only current way to use them is to put them in the current working directory of your application. That is, **WHEREVER YOU RUN YOUR NODE APPLICATION**

*   llama-xxxx.dll is required.
*   According to whatever model you are using, you'll need to select the proper model loader.
    *   For example, if you running an Mosaic MPT model, you will need to select the mpt-(buildvariant).(dynamiclibrary)

### Test

```sh
yarn test
```

### Source Overview

#### src/

*   Extra functions to help aid devex
*   Typings for the native node addon
*   the javascript interface

#### test/

*   simple unit testings for some functions exported.
*   more advanced ai testing is not handled

#### spec/

*   Average look and feel of the api
*   Should work assuming a model and libraries are installed locally in working directory

#### index.cc

*   The bridge between nodejs and c. Where the bindings are.

#### prompt.cc

*   Handling prompting and inference of models in a threadsafe, asynchronous way.

### Known Issues

*   why your model may be spewing bull 💩
    *   The downloaded model is broken (just reinstall or download from official site)
*   Your model is hanging after a call to generate tokens.
    *   Is `nPast` set too high? This may cause your model to hang (03/16/2024), Linux Mint, Ubuntu 22.04
*   Your GPU usage is still high after node.js exits.
    *   Make sure to call `model.dispose()`!!!

### Roadmap

This package has been stabilizing over time development, and breaking changes may happen until the api stabilizes. Here's what's the todo list:

*   \[ ] Purely offline. Per the gui, which can be run completely offline, the bindings should be as well.
*   \[ ] NPM bundle size reduction via optionalDependencies strategy (need help)
    *   Should include prebuilds to avoid painful node-gyp errors
*   \[x] createChatSession ( the python equivalent to create\_chat\_session )
*   \[x] generateTokens, the new name for createTokenStream. As of 3.2.0, this is released but not 100% tested. Check spec/generator.mjs!
*   \[x] ~~createTokenStream, an async iterator that streams each token emitted from the model. Planning on following this [example](https://github.com/nodejs/node-addon-examples/tree/main/threadsafe-async-iterator)~~ May not implement unless someone else can complete
*   \[x] prompt models via a threadsafe function in order to have proper non blocking behavior in nodejs
*   \[x] generateTokens is the new name for this^
*   \[x] proper unit testing (integrate with circle ci)
*   \[x] publish to npm under alpha tag `gpt4all@alpha`
*   \[x] have more people test on other platforms (mac tester needed)
*   \[x] switch to new pluggable backend

### API Reference

<!-- Generated by documentation.js. Update this documentation by updating the source code. -->

##### Table of Contents

*   [type](#type)
*   [TokenCallback](#tokencallback)
*   [ChatSessionOptions](#chatsessionoptions)
    *   [systemPrompt](#systemprompt)
    *   [messages](#messages)
*   [initialize](#initialize)
    *   [Parameters](#parameters)
*   [generate](#generate)
    *   [Parameters](#parameters-1)
*   [InferenceModel](#inferencemodel)
    *   [createChatSession](#createchatsession)
        *   [Parameters](#parameters-2)
    *   [generate](#generate-1)
        *   [Parameters](#parameters-3)
    *   [dispose](#dispose)
*   [EmbeddingModel](#embeddingmodel)
    *   [dispose](#dispose-1)
*   [InferenceResult](#inferenceresult)
*   [LLModel](#llmodel)
    *   [constructor](#constructor)
        *   [Parameters](#parameters-4)
    *   [type](#type-1)
    *   [name](#name)
    *   [stateSize](#statesize)
    *   [threadCount](#threadcount)
    *   [setThreadCount](#setthreadcount)
        *   [Parameters](#parameters-5)
    *   [infer](#infer)
        *   [Parameters](#parameters-6)
    *   [embed](#embed)
        *   [Parameters](#parameters-7)
    *   [isModelLoaded](#ismodelloaded)
    *   [setLibraryPath](#setlibrarypath)
        *   [Parameters](#parameters-8)
    *   [getLibraryPath](#getlibrarypath)
    *   [initGpuByString](#initgpubystring)
        *   [Parameters](#parameters-9)
    *   [hasGpuDevice](#hasgpudevice)
    *   [listGpu](#listgpu)
        *   [Parameters](#parameters-10)
    *   [dispose](#dispose-2)
*   [GpuDevice](#gpudevice)
    *   [type](#type-2)
*   [LoadModelOptions](#loadmodeloptions)
    *   [modelPath](#modelpath)
    *   [librariesPath](#librariespath)
    *   [modelConfigFile](#modelconfigfile)
    *   [allowDownload](#allowdownload)
    *   [verbose](#verbose)
    *   [device](#device)
    *   [nCtx](#nctx)
    *   [ngl](#ngl)
*   [loadModel](#loadmodel)
    *   [Parameters](#parameters-11)
*   [InferenceProvider](#inferenceprovider)
*   [createCompletion](#createcompletion)
    *   [Parameters](#parameters-12)
*   [createCompletionStream](#createcompletionstream)
    *   [Parameters](#parameters-13)
*   [createCompletionGenerator](#createcompletiongenerator)
    *   [Parameters](#parameters-14)
*   [createEmbedding](#createembedding)
    *   [Parameters](#parameters-15)
*   [CompletionOptions](#completionoptions)
    *   [verbose](#verbose-1)
    *   [onToken](#ontoken)
*   [Message](#message)
    *   [role](#role)
    *   [content](#content)
*   [prompt\_tokens](#prompt_tokens)
*   [completion\_tokens](#completion_tokens)
*   [total\_tokens](#total_tokens)
*   [n\_past\_tokens](#n_past_tokens)
*   [CompletionReturn](#completionreturn)
    *   [model](#model)
    *   [usage](#usage)
    *   [message](#message-1)
*   [CompletionStreamReturn](#completionstreamreturn)
*   [LLModelPromptContext](#llmodelpromptcontext)
    *   [logitsSize](#logitssize)
    *   [tokensSize](#tokenssize)
    *   [nPast](#npast)
    *   [nPredict](#npredict)
    *   [promptTemplate](#prompttemplate)
    *   [nCtx](#nctx-1)
    *   [topK](#topk)
    *   [topP](#topp)
    *   [minP](#minp)
    *   [temperature](#temperature)
    *   [nBatch](#nbatch)
    *   [repeatPenalty](#repeatpenalty)
    *   [repeatLastN](#repeatlastn)
    *   [contextErase](#contexterase)
*   [DEFAULT\_DIRECTORY](#default_directory)
*   [DEFAULT\_LIBRARIES\_DIRECTORY](#default_libraries_directory)
*   [DEFAULT\_MODEL\_CONFIG](#default_model_config)
*   [DEFAULT\_PROMPT\_CONTEXT](#default_prompt_context)
*   [DEFAULT\_MODEL\_LIST\_URL](#default_model_list_url)
*   [downloadModel](#downloadmodel)
    *   [Parameters](#parameters-16)
    *   [Examples](#examples)
*   [DownloadModelOptions](#downloadmodeloptions)
    *   [modelPath](#modelpath-1)
    *   [verbose](#verbose-2)
    *   [url](#url)
    *   [md5sum](#md5sum)
*   [DownloadController](#downloadcontroller)
    *   [cancel](#cancel)
    *   [promise](#promise)

#### type

Model architecture. This argument currently does not have any functionality and is just used as descriptive identifier for user.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

#### TokenCallback

Callback for controlling token generation. Return false to stop token generation.

Type: function (tokenId: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number), token: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String), total: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)): [boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean)

#### ChatSessionOptions

**Extends Partial\<LLModelPromptContext>**

Options for the chat session.

##### systemPrompt

System prompt to ingest on initialization.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

##### messages

Messages to ingest on initialization.

Type: [Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)<[Message](#message)>

#### initialize

Ingests system prompt and initial messages.
Sets this chat session as the active chat session of the model.

##### Parameters

*   `options` **[ChatSessionOptions](#chatsessionoptions)** The options for the chat session.

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)\<void>**&#x20;

#### generate

Prompts the model in chat-session context.

##### Parameters

*   `prompt` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** The prompt input.
*   `options` **[CompletionOptions](#completionoptions)?** Prompt context and other options.
*   `callback` **[TokenCallback](#tokencallback)?** Token generation callback.

<!---->

*   Throws **[Error](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Error)** If the chat session is not the active chat session of the model.

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)<[CompletionReturn](#completionreturn)>** The model's response to the prompt.

#### InferenceModel

InferenceModel represents an LLM which can make chat predictions, similar to GPT transformers.

##### createChatSession

Create a chat session with the model.

###### Parameters

*   `options` **[ChatSessionOptions](#chatsessionoptions)?** The options for the chat session.

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)\<ChatSession>** The chat session.

##### generate

Prompts the model with a given input and optional parameters.

###### Parameters

*   `prompt` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)**&#x20;
*   `options` **[CompletionOptions](#completionoptions)?** Prompt context and other options.
*   `callback` **[TokenCallback](#tokencallback)?** Token generation callback.
*   `input`  The prompt input.

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)<[CompletionReturn](#completionreturn)>** The model's response to the prompt.

##### dispose

delete and cleanup the native model

Returns **void**&#x20;

#### EmbeddingModel

EmbeddingModel represents an LLM which can create embeddings, which are float arrays

##### dispose

delete and cleanup the native model

Returns **void**&#x20;

#### InferenceResult

Shape of LLModel's inference result.

#### LLModel

LLModel class representing a language model.
This is a base class that provides common functionality for different types of language models.

##### constructor

Initialize a new LLModel.

###### Parameters

*   `path` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** Absolute path to the model file.

<!---->

*   Throws **[Error](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Error)** If the model file does not exist.

##### type

undefined or user supplied

Returns **([string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String) | [undefined](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/undefined))**&#x20;

##### name

The name of the model.

Returns **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)**&#x20;

##### stateSize

Get the size of the internal state of the model.
NOTE: This state data is specific to the type of model you have created.

Returns **[number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)** the size in bytes of the internal state of the model

##### threadCount

Get the number of threads used for model inference.
The default is the number of physical cores your computer has.

Returns **[number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)** The number of threads used for model inference.

##### setThreadCount

Set the number of threads used for model inference.

###### Parameters

*   `newNumber` **[number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)** The new number of threads.

Returns **void**&#x20;

##### infer

Prompt the model with a given input and optional parameters.
This is the raw output from model.
Use the prompt function exported for a value

###### Parameters

*   `prompt` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** The prompt input.
*   `promptContext` **Partial<[LLModelPromptContext](#llmodelpromptcontext)>** Optional parameters for the prompt context.
*   `callback` **[TokenCallback](#tokencallback)?** optional callback to control token generation.

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)<[InferenceResult](#inferenceresult)>** The result of the model prompt.

##### embed

Embed text with the model. Keep in mind that
Use the prompt function exported for a value

###### Parameters

*   `text` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** The prompt input.

Returns **[Float32Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Float32Array)** The result of the model prompt.

##### isModelLoaded

Whether the model is loaded or not.

Returns **[boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean)**&#x20;

##### setLibraryPath

Where to search for the pluggable backend libraries

###### Parameters

*   `s` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)**&#x20;

Returns **void**&#x20;

##### getLibraryPath

Where to get the pluggable backend libraries

Returns **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)**&#x20;

##### initGpuByString

Initiate a GPU by a string identifier.

###### Parameters

*   `memory_required` **[number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)** Should be in the range size\_t or will throw
*   `device_name` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** 'amd' | 'nvidia' | 'intel' | 'gpu' | gpu name.
    read LoadModelOptions.device for more information

Returns **[boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean)**&#x20;

##### hasGpuDevice

From C documentation

Returns **[boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean)** True if a GPU device is successfully initialized, false otherwise.

##### listGpu

GPUs that are usable for this LLModel

###### Parameters

*   `nCtx` **[number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)** Maximum size of context window

<!---->

*   Throws **any** if hasGpuDevice returns false (i think)

Returns **[Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)<[GpuDevice](#gpudevice)>**&#x20;

##### dispose

delete and cleanup the native model

Returns **void**&#x20;

#### GpuDevice

an object that contains gpu data on this machine.

##### type

same as VkPhysicalDeviceType

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

#### LoadModelOptions

Options that configure a model's behavior.

##### modelPath

Where to look for model files.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

##### librariesPath

Where to look for the backend libraries.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

##### modelConfigFile

The path to the model configuration file, useful for offline usage or custom model configurations.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

##### allowDownload

Whether to allow downloading the model if it is not present at the specified path.

Type: [boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean)

##### verbose

Enable verbose logging.

Type: [boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean)

##### device

The processing unit on which the model will run. It can be set to

*   "cpu": Model will run on the central processing unit.
*   "gpu": Model will run on the best available graphics processing unit, irrespective of its vendor.
*   "amd", "nvidia", "intel": Model will run on the best available GPU from the specified vendor.
*   "gpu name": Model will run on the GPU that matches the name if it's available.
    Note: If a GPU device lacks sufficient RAM to accommodate the model, an error will be thrown, and the GPT4All
    instance will be rendered invalid. It's advised to ensure the device has enough memory before initiating the
    model.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

##### nCtx

The Maximum window size of this model

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### ngl

Number of gpu layers needed

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

#### loadModel

Loads a machine learning model with the specified name. The defacto way to create a model.
By default this will download a model from the official GPT4ALL website, if a model is not present at given path.

##### Parameters

*   `modelName` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** The name of the model to load.
*   `options` **([LoadModelOptions](#loadmodeloptions) | [undefined](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/undefined))?** (Optional) Additional options for loading the model.

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)<([InferenceModel](#inferencemodel) | [EmbeddingModel](#embeddingmodel))>** A promise that resolves to an instance of the loaded LLModel.

#### InferenceProvider

Interface for inference, implemented by InferenceModel and ChatSession.

#### createCompletion

The nodejs equivalent to python binding's chat\_completion

##### Parameters

*   `provider` **[InferenceProvider](#inferenceprovider)** The inference model object or chat session
*   `message` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** The user input message
*   `options` **[CompletionOptions](#completionoptions)** The options for creating the completion.

Returns **[CompletionReturn](#completionreturn)** The completion result.

#### createCompletionStream

Streaming variant of createCompletion, returns a stream of tokens and a promise that resolves to the completion result.

##### Parameters

*   `provider` **[InferenceProvider](#inferenceprovider)** The inference model object or chat session
*   `message` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** The user input message.
*   `options` **[CompletionOptions](#completionoptions)** The options for creating the completion.

Returns **[CompletionStreamReturn](#completionstreamreturn)** An object of token stream and the completion result promise.

#### createCompletionGenerator

Creates an async generator of tokens

##### Parameters

*   `provider` **[InferenceProvider](#inferenceprovider)** The inference model object or chat session
*   `message` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** The user input message.
*   `options` **[CompletionOptions](#completionoptions)** The options for creating the completion.

Returns **AsyncGenerator<[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)>** The stream of generated tokens

#### createEmbedding

The nodejs moral equivalent to python binding's Embed4All().embed()
meow

##### Parameters

*   `model` **[EmbeddingModel](#embeddingmodel)** The language model object.
*   `text` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** text to embed

Returns **[Float32Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Float32Array)** The completion result.

#### CompletionOptions

**Extends Partial\<LLModelPromptContext>**

The options for creating the completion.

##### verbose

Indicates if verbose logging is enabled.

Type: [boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean)

##### onToken

Callback for controlling token generation. Return false to stop processing.

Type: [TokenCallback](#tokencallback)

#### Message

A message in the conversation.

##### role

The role of the message.

Type: (`"system"` | `"assistant"` | `"user"`)

##### content

The message content.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

#### prompt\_tokens

The number of tokens used in the prompt. Currently not available and always 0.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

#### completion\_tokens

The number of tokens used in the completion.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

#### total\_tokens

The total number of tokens used. Currently not available and always 0.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

#### n\_past\_tokens

Number of tokens used in the conversation.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

#### CompletionReturn

The result of a completion.

##### model

The model used for the completion.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

##### usage

Token usage report.

Type: {prompt\_tokens: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number), completion\_tokens: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number), total\_tokens: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number), n\_past\_tokens: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)}

##### message

The generated completion.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

#### CompletionStreamReturn

The result of a streamed completion, containing a stream of tokens and a promise that resolves to the completion result.

#### LLModelPromptContext

Model inference arguments for generating completions.

##### logitsSize

The size of the raw logits vector.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### tokensSize

The size of the raw tokens vector.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### nPast

The number of tokens in the past conversation.
This controls how far back the model looks when generating completions.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### nPredict

The maximum number of tokens to predict.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### promptTemplate

Template for user / assistant message pairs.
%1 is required and will be replaced by the user input.
%2 is optional and will be replaced by the assistant response.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

##### nCtx

The context window size. Do not use, it has no effect. See loadModel options.
THIS IS DEPRECATED!!!
Use loadModel's nCtx option instead.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### topK

The top-k logits to sample from.
Top-K sampling selects the next token only from the top K most likely tokens predicted by the model.
It helps reduce the risk of generating low-probability or nonsensical tokens, but it may also limit
the diversity of the output. A higher value for top-K (eg., 100) will consider more tokens and lead
to more diverse text, while a lower value (eg., 10) will focus on the most probable tokens and generate
more conservative text. 30 - 60 is a good range for most tasks.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### topP

The nucleus sampling probability threshold.
Top-P limits the selection of the next token to a subset of tokens with a cumulative probability
above a threshold P. This method, also known as nucleus sampling, finds a balance between diversity
and quality by considering both token probabilities and the number of tokens available for sampling.
When using a higher value for top-P (eg., 0.95), the generated text becomes more diverse.
On the other hand, a lower value (eg., 0.1) produces more focused and conservative text.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### minP

The minimum probability of a token to be considered.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### temperature

The temperature to adjust the model's output distribution.
Temperature is like a knob that adjusts how creative or focused the output becomes. Higher temperatures
(eg., 1.2) increase randomness, resulting in more imaginative and diverse text. Lower temperatures (eg., 0.5)
make the output more focused, predictable, and conservative. When the temperature is set to 0, the output
becomes completely deterministic, always selecting the most probable next token and producing identical results
each time. A safe range would be around 0.6 - 0.85, but you are free to search what value fits best for you.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### nBatch

The number of predictions to generate in parallel.
By splitting the prompt every N tokens, prompt-batch-size reduces RAM usage during processing. However,
this can increase the processing time as a trade-off. If the N value is set too low (e.g., 10), long prompts
with 500+ tokens will be most affected, requiring numerous processing runs to complete the prompt processing.
To ensure optimal performance, setting the prompt-batch-size to 2048 allows processing of all tokens in a single run.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### repeatPenalty

The penalty factor for repeated tokens.
Repeat-penalty can help penalize tokens based on how frequently they occur in the text, including the input prompt.
A token that has already appeared five times is penalized more heavily than a token that has appeared only one time.
A value of 1 means that there is no penalty and values larger than 1 discourage repeated tokens.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### repeatLastN

The number of last tokens to penalize.
The repeat-penalty-tokens N option controls the number of tokens in the history to consider for penalizing repetition.
A larger value will look further back in the generated text to prevent repetitions, while a smaller value will only
consider recent tokens.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

##### contextErase

The percentage of context to erase if the context window is exceeded.

Type: [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number)

#### DEFAULT\_DIRECTORY

From python api:
models will be stored in (homedir)/.cache/gpt4all/\`

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

#### DEFAULT\_LIBRARIES\_DIRECTORY

From python api:
The default path for dynamic libraries to be stored.
You may separate paths by a semicolon to search in multiple areas.
This searches DEFAULT\_DIRECTORY/libraries, cwd/libraries, and finally cwd.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

#### DEFAULT\_MODEL\_CONFIG

Default model configuration.

Type: ModelConfig

#### DEFAULT\_PROMPT\_CONTEXT

Default prompt context.

Type: [LLModelPromptContext](#llmodelpromptcontext)

#### DEFAULT\_MODEL\_LIST\_URL

Default model list url.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

#### downloadModel

Initiates the download of a model file.
By default this downloads without waiting. use the controller returned to alter this behavior.

##### Parameters

*   `modelName` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** The model to be downloaded.
*   `options` **[DownloadModelOptions](#downloadmodeloptions)** to pass into the downloader. Default is { location: (cwd), verbose: false }.

##### Examples

```javascript
const download = downloadModel('ggml-gpt4all-j-v1.3-groovy.bin')
download.promise.then(() => console.log('Downloaded!'))
```

*   Throws **[Error](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Error)** If the model already exists in the specified location.
*   Throws **[Error](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Error)** If the model cannot be found at the specified url.

Returns **[DownloadController](#downloadcontroller)** object that allows controlling the download process.

#### DownloadModelOptions

Options for the model download process.

##### modelPath

location to download the model.
Default is process.cwd(), or the current working directory

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

##### verbose

Debug mode -- check how long it took to download in seconds

Type: [boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean)

##### url

Remote download url. Defaults to `https://gpt4all.io/models/gguf/<modelName>`

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

##### md5sum

MD5 sum of the model file. If this is provided, the downloaded file will be checked against this sum.
If the sums do not match, an error will be thrown and the file will be deleted.

Type: [string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)

#### DownloadController

Model download controller.

##### cancel

Cancel the request to download if this is called.

Type: function (): void

##### promise

A promise resolving to the downloaded models config once the download is done

Type: [Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)\<ModelConfig>
