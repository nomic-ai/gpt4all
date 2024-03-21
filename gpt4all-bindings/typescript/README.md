# GPT4All Node.js API

Native Node.js LLM bindings for all.

```sh
yarn add gpt4all@latest

npm install gpt4all@latest

pnpm install gpt4all@latest

```
## Breaking changes in version 4!!
*   See [Transition](#changes)
## Contents 
*   See [API Reference](#api-reference)
*   See [Examples](#api-example)
*   See [Developing](#develop)
*   GPT4ALL nodejs bindings created by [jacoobes](https://github.com/jacoobes), [limez](https://github.com/iimez) and the [nomic ai community](https://home.nomic.ai), for all to use.
*   [spare change](https://github.com/sponsors/jacoobes) for a college student? 🤑
## Api Example
### Chat Completion
```js
import { createCompletion, loadModel } from '../src/gpt4all.js'

const model = await loadModel('mistral-7b-openorca.gguf2.Q4_0.gguf', { verbose: true, device: 'gpu' });

const completion1 = await createCompletion(model, 'What is 1 + 1?', { verbose: true, })
console.log(completion1.message)

const completion2 = await createCompletion(model, 'And if we add two?', {  verbose: true  })
console.log(completion2.message)

model.dispose()
```

### Embedding

```js
import { loadModel, createEmbedding } from '../src/gpt4all.js'

const embedder = await loadModel("nomic-embed-text-v1.5.f16.gguf", { verbose: true, type: 'embedding'})

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
### Offline usage 
do this b4 going offline
```sh
curl -L https://gpt4all.io/models/models3.json -o ./models3.json
```
```js 
import { createCompletion, loadModel } from 'gpt4all'

//make sure u downloaded the models before going offline!
const model = await loadModel('mistral-7b-openorca.gguf2.Q4_0.gguf', { 
    verbose: true,
    device: 'gpu',
    modelConfigFile: "./models3.json" 
});

await createCompletion(model, 'What is 1 + 1?', { verbose: true })

model.dispose();
```

## Develop
### Build Instructions

*   `binding.gyp` is compile config
*   Tested on Ubuntu. Everything seems to work fine
*   Tested on Windows. Everything works fine.
*   Sparse testing on mac os.
*   MingW script works to build the gpt4all-backend. We left it there just in case. **HOWEVER**, this package works only with MSVC built dlls.

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
node scripts/prebuild.js
```
*   llama.cpp git submodule for gpt4all can be possibly absent. If this is the case, make sure to run in llama.cpp parent directory

```sh
git submodule update --init --recursive
```

```sh
yarn build:backend
```
This will build platform-dependent dynamic libraries, and will be located in runtimes/(platform)/native 

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
    * Is `nPast` set too high? This may cause your model to hang (03/16/2024), Linux Mint, Ubuntu 22.04
*  Your GPU usage is still high after node.js exits.
    * Make sure to call `model.dispose()`!!!

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

## Changes
This repository serves as the new bindings for nodejs users.
- If you were a user of [these bindings](https://github.com/nomic-ai/gpt4all-ts), they are outdated.
- Version 4 includes the follow breaking changes
    * `createEmbedding` & `EmbeddingModel.embed()` returns an object, EmbeddingResult, instead of a float32array.
    * 

### API Reference
