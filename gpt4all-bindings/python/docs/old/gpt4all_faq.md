# GPT4All FAQ

## What models are supported by the GPT4All ecosystem?

Currently, there are six different model architectures that are supported:

1. GPT-J - Based off of the GPT-J architecture with examples found [here](https://huggingface.co/EleutherAI/gpt-j-6b)
2. LLaMA - Based off of the LLaMA architecture with examples found [here](https://huggingface.co/models?sort=downloads&search=llama)
3. MPT - Based off of Mosaic ML's MPT architecture with examples found [here](https://huggingface.co/mosaicml/mpt-7b)
4. Replit - Based off of Replit Inc.'s Replit architecture with examples found [here](https://huggingface.co/replit/replit-code-v1-3b)
5. Falcon - Based off of TII's Falcon architecture with examples found [here](https://huggingface.co/tiiuae/falcon-40b)
6. StarCoder - Based off of BigCode's StarCoder architecture with examples found [here](https://huggingface.co/bigcode/starcoder)

## Why so many different architectures? What differentiates them?

One of the major differences is license. Currently, the LLaMA based models are subject to a non-commercial license, whereas the GPTJ and MPT base
models allow commercial usage. However, its successor [Llama 2 is commercially licensable](https://ai.meta.com/llama/license/), too. In the early
advent of the recent explosion of activity in open source local models, the LLaMA models have generally been seen as performing better, but that is
changing quickly. Every week - even every day! - new models are released with some of the GPTJ and MPT models competitive in performance/quality with
LLaMA. What's more, there are some very nice architectural innovations with the MPT models that could lead to new performance/quality gains.

## How does GPT4All make these models available for CPU inference?

By leveraging the ggml library written by Georgi Gerganov and a growing community of developers. There are currently multiple different versions of
this library. The original GitHub repo can be found [here](https://github.com/ggerganov/ggml), but the developer of the library has also created a
LLaMA based version [here](https://github.com/ggerganov/llama.cpp). Currently, this backend is using the latter as a submodule.

## Does that mean GPT4All is compatible with all llama.cpp models and vice versa?

Yes!

The upstream [llama.cpp](https://github.com/ggerganov/llama.cpp) project has introduced several [compatibility breaking] quantization methods recently.
This is a breaking change that renders all previous models (including the ones that GPT4All uses) inoperative with newer versions of llama.cpp since
that change.

Fortunately, we have engineered a submoduling system allowing us to dynamically load different versions of the underlying library so that
GPT4All just works.

[compatibility breaking]: https://github.com/ggerganov/llama.cpp/commit/b9fd7eee57df101d4a3e3eabc9fd6c2cb13c9ca1

## What are the system requirements?

Your CPU needs to support [AVX or AVX2 instructions](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions) and you need enough RAM to load a model into memory.

## What about GPU inference?

In newer versions of llama.cpp, there has been some added support for NVIDIA GPU's for inference. We're investigating how to incorporate this into our downloadable installers.

## Ok, so bottom line... how do I make my model on Hugging Face compatible with GPT4All ecosystem right now?

1. Check to make sure the Hugging Face model is available in one of our three supported architectures
2. If it is, then you can use the conversion script inside of our pinned llama.cpp submodule for GPTJ and LLaMA based models
3. Or if your model is an MPT model you can use the conversion script located directly in this backend directory under the scripts subdirectory 

## Language Bindings

#### There's a problem with the download

Some bindings can download a model, if allowed to do so. For example, in Python or TypeScript if `allow_download=True`
or `allowDownload=true` (default), a model is automatically downloaded into `.cache/gpt4all/` in the user's home folder,
unless it already exists.

In case of connection issues or errors during the download, you might want to manually verify the model file's MD5
checksum by comparing it with the one listed in [models3.json].

As an alternative to the basic downloader built into the bindings, you can choose to download from the 
<https://gpt4all.io/> website instead. Scroll down to 'Model Explorer' and pick your preferred model.

[models3.json]: https://github.com/nomic-ai/gpt4all/blob/main/gpt4all-chat/metadata/models3.json

#### I need the chat GUI and bindings to behave the same

The chat GUI and bindings are based on the same backend. You can make them behave the same way by following these steps:

- First of all, ensure that all parameters in the chat GUI settings match those passed to the generating API, e.g.:

    === "Python"
        ``` py
        from gpt4all import GPT4All
        model = GPT4All(...)
        model.generate("prompt text", temp=0, ...)  # adjust parameters
        ```
    === "TypeScript"
        ``` ts
        import { createCompletion, loadModel } from '../src/gpt4all.js'
        const ll = await loadModel(...);
        const messages = ...
        const re = await createCompletion(ll, messages, { temp: 0, ... });  // adjust parameters
        ```

- To make comparing the output easier, set _Temperature_ in both to 0 for now. This will make the output deterministic.

- Next you'll have to compare the templates, adjusting them as necessary, based on how you're using the bindings.
    - Specifically, in Python:
        - With simple `generate()` calls, the input has to be surrounded with system and prompt templates.
        - When using a chat session, it depends on whether the bindings are allowed to download [models3.json]. If yes,
          and in the chat GUI the default templates are used, it'll be handled automatically. If no, use
          `chat_session()` template parameters to customize them.

- Once you're done, remember to reset _Temperature_ to its previous value in both chat GUI and your custom code.
