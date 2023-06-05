### Javascript Bindings
The original [GPT4All typescript bindings](https://github.com/nomic-ai/gpt4all-ts) are now out of date.

- created by [jacoobes](https://github.com/jacoobes) and [nomic ai](https://home.nomic.ai) :D, for all to use.
- will maintain this repository when possible, new feature requests will be handled through nomic

### Build Instructions

- As of 05/21/2023, Tested on windows (MSVC) only. (somehow got it to work on MSVC 🤯)
    - binding.gyp is compile config

### Requirements
- git
- [node.js >= 18.0.0](https://nodejs.org/en)
- [yarn](https://yarnpkg.com/)
- [node-gyp](https://github.com/nodejs/node-gyp)
    - all of its requirements.

### Build
```sh
git clone https://github.com/nomic-ai/gpt4all.git
cd gpt4all-bindings/typescript
```
- The below shell commands assume the current working directory is `typescript`.

- To Build and Rebuild: 
 ```sh
 yarn
 ```
 - llama.cpp git submodule for gpt4all can be possibly absent. If this is the case, make sure to run in llama.cpp parent directory
 ```sh
git submodule update --init --depth 1 --recursive
 ```
**AS OF NEW BACKEND** to build the backend,
```sh
yarn build:backend
```
This will build platform-dependent dynamic libraries, and will be located in runtimes/(platform)/native The only current way to use them is to put them in the current working directory of your application. That is, **WHEREVER YOU RUN YOUR NODE APPLICATION**
- llama-xxxx.dll is required.
- According to whatever model you are using, you'll need to select the proper model loader.
    - For example, if you running an Mosaic MPT model, you will need to select the mpt-(buildvariant).(dynamiclibrary)

### Test
```sh
yarn test
```
### Source Overview

#### src/
- Extra functions to help aid devex
- Typings for the native node addon
- the javascript interface

#### test/
- simple unit testings for some functions exported.
- more advanced ai testing is not handled 

#### spec/
- Average look and feel of the api
- Should work assuming a model is installed locally in working directory

#### index.cc
- The bridge between nodejs and c. Where the bindings are.

### Roadmap
This package is in active development, and breaking changes may happen until the api stabilizes. Here's what's the todo list:

- [ ] prompt models via a threadsafe function in order to have proper non blocking behavior in nodejs
- [ ] createTokenStream, an async iterator that streams each token emitted from the model. Planning on following this [example](https://github.com/nodejs/node-addon-examples/tree/main/threadsafe-async-iterator)
- [ ] proper unit testing
- [ ] publish to npm under alpha tag `gpt4all@alpha`
- [ ] have more people test on other platforms (mac tester needed)
- [x] switch to new pluggable backend

