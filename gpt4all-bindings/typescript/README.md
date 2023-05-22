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
git submodule update --init --depth 1 --recursive`
 ```
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


