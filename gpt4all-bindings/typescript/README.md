# GPT4All Node.js API

Native Node.js LLM bindings for all.

```sh
yarn add gpt4all@latest

npm install gpt4all@latest

pnpm install gpt4all@latest

```

The original [GPT4All typescript bindings](https://github.com/nomic-ai/gpt4all-ts) are now out of date.

*   New bindings created by [jacoobes](https://github.com/jacoobes), [limez](https://github.com/iimez) and the [nomic ai community](https://home.nomic.ai), for all to use.
*   The nodejs api has made strides to mirror the python api. It is not 100% mirrored, but many pieces of the api resemble its python counterpart.
*   Everything should work out the box.
*   See [API Reference](#api-reference)

### Chat Completion

```js
import { createCompletion, loadModel } from '../src/gpt4all.js'

const model = await loadModel('mistral-7b-openorca.Q4_0.gguf', { verbose: true });

const response = await createCompletion(model, [
    { role : 'system', content: 'You are meant to be annoying and unhelpful.'  },
    { role : 'user', content: 'What is 1 + 1?'  } 
]);

```

### Embedding

```js
import { createEmbedding, loadModel } from '../src/gpt4all.js'

const model = await loadModel('ggml-all-MiniLM-L6-v2-f16', { verbose: true });

const fltArray = createEmbedding(model, "Pain is inevitable, suffering optional");
```

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
    *   That's it so far

### Roadmap

This package is in active development, and breaking changes may happen until the api stabilizes. Here's what's the todo list:

*   \[x] prompt models via a threadsafe function in order to have proper non blocking behavior in nodejs
*   \[ ] ~~createTokenStream, an async iterator that streams each token emitted from the model. Planning on following this [example](https://github.com/nodejs/node-addon-examples/tree/main/threadsafe-async-iterator)~~ May not implement unless someone else can complete
*   \[x] proper unit testing (integrate with circle ci)
*   \[x] publish to npm under alpha tag `gpt4all@alpha`
*   \[x] have more people test on other platforms (mac tester needed)
*   \[x] switch to new pluggable backend
*   \[ ] NPM bundle size reduction via optionalDependencies strategy (need help)
    *   Should include prebuilds to avoid painful node-gyp errors
*   \[ ] createChatSession ( the python equivalent to create\_chat\_session )

### API Reference
