"use strict";

/// This file implements the gpt4all.d.ts file endings.
/// Written in commonjs to support both ESM and CJS projects.
const { existsSync } = require("node:fs");
const path = require("node:path");
const Stream = require("node:stream");
const assert = require("node:assert");
const { LLModel } = require("node-gyp-build")(path.resolve(__dirname, ".."));
const {
    retrieveModel,
    downloadModel,
    appendBinSuffixIfMissing,
} = require("./util.js");
const {
    DEFAULT_DIRECTORY,
    DEFAULT_LIBRARIES_DIRECTORY,
    DEFAULT_PROMPT_CONTEXT,
    DEFAULT_MODEL_CONFIG,
    DEFAULT_MODEL_LIST_URL,
} = require("./config.js");
const { InferenceModel, EmbeddingModel } = require("./models.js");
const { ChatSession } = require("./chat-session.js");

/**
 * Loads a machine learning model with the specified name. The defacto way to create a model.
 * By default this will download a model from the official GPT4ALL website, if a model is not present at given path.
 *
 * @param {string} modelName - The name of the model to load.
 * @param {import('./gpt4all').LoadModelOptions|undefined} [options] - (Optional) Additional options for loading the model.
 * @returns {Promise<InferenceModel | EmbeddingModel>} A promise that resolves to an instance of the loaded LLModel.
 */
async function loadModel(modelName, options = {}) {
    const loadOptions = {
        modelPath: DEFAULT_DIRECTORY,
        librariesPath: DEFAULT_LIBRARIES_DIRECTORY,
        type: "inference",
        allowDownload: true,
        verbose: false,
        device: "cpu",
        nCtx: 2048,
        ngl: 100,
        ...options,
    };

    const modelConfig = await retrieveModel(modelName, {
        modelPath: loadOptions.modelPath,
        modelConfigFile: loadOptions.modelConfigFile,
        allowDownload: loadOptions.allowDownload,
        verbose: loadOptions.verbose,
    });

    assert.ok(
        typeof loadOptions.librariesPath === "string",
        "Libraries path should be a string"
    );
    const existingPaths = loadOptions.librariesPath
        .split(";")
        .filter(existsSync)
        .join(";");

    const llmOptions = {
        model_name: appendBinSuffixIfMissing(modelName),
        model_path: loadOptions.modelPath,
        library_path: existingPaths,
        device: loadOptions.device,
        nCtx: loadOptions.nCtx,
        ngl: loadOptions.ngl,
    };

    if (loadOptions.verbose) {
        console.debug("Creating LLModel:", {
            llmOptions,
            modelConfig,
        });
    }
    const llmodel = new LLModel(llmOptions);
    if (loadOptions.type === "embedding") {
        return new EmbeddingModel(llmodel, modelConfig);
    } else if (loadOptions.type === "inference") {
        return new InferenceModel(llmodel, modelConfig);
    } else {
        throw Error("Invalid model type: " + loadOptions.type);
    }
}

function createEmbedding(model, text, options={}) {
    let {
        dimensionality = undefined,
        long_text_mode = "mean",
        atlas = false,
    } = options;

    if (dimensionality === undefined) {
        dimensionality = -1;
    } else {
        if (dimensionality <= 0) {
            throw new Error(
                `Dimensionality must be undefined or a positive integer, got ${dimensionality}`
            );
        }
        if (dimensionality < model.MIN_DIMENSIONALITY) {
            console.warn(
                `Dimensionality ${dimensionality} is less than the suggested minimum of ${model.MIN_DIMENSIONALITY}. Performance may be degraded.`
            );
        }
    }

    let do_mean;
    switch (long_text_mode) {
        case "mean":
            do_mean = true;
            break;
        case "truncate":
            do_mean = false;
            break;
        default:
            throw new Error(
                `Long text mode must be one of 'mean' or 'truncate', got ${long_text_mode}`
            );
    }
    /**
   text = info[0]
   auto prefix = info[1].As<Napi::String>().Utf8Value();
   auto dimensionality = info[2].As<Napi::Number>().Int32Value();
   auto do_mean = info[3].As<Napi::Boolean>().Value();
   auto atlas = info[4].As<Napi::Boolean>().Value();
  */
    return model.embed(text, options?.prefix, dimensionality, do_mean, atlas);
}

const defaultCompletionOptions = {
    verbose: false,
    ...DEFAULT_PROMPT_CONTEXT,
};

async function createCompletion(
    provider,
    input,
    options = defaultCompletionOptions
) {
    const completionOptions = {
        ...defaultCompletionOptions,
        ...options,
    };

    const response = await provider.generate(
        input,
        completionOptions,
        options.onToken
    );

    return {
        model: provider.modelName,
        usage: {
            // TODO find a way to determine input token count reliably.
            // using nPastDelta fails when the context window is exceeded.
            // prompt_tokens: nPastDelta - tokensGenerated,
            // total_tokens: nPastDelta,
            prompt_tokens: 0,
            total_tokens: 0,
            completion_tokens: response.tokensGenerated,
            n_past_tokens: response.nPast,
        },
        choices: [
            {
                message: {
                    role: "assistant",
                    content: response.text,
                },
                // index: 0,
                // logprobs: null,
                // finish_reason: "length",
            },
        ],
    };
}

function createCompletionStream(
    provider,
    input,
    options = defaultCompletionOptions
) {
    const completionStream = new Stream.PassThrough({
        encoding: "utf-8",
    });

    const completionPromise = createCompletion(provider, input, {
        ...options,
        onToken: (tokenId, text, fullText) => {
            completionStream.push(text);
            if (options.onToken) {
                return options.onToken(tokenId, text, fullText);
            }
        },
    }).then((result) => {
        completionStream.push(null);
        completionStream.emit("end");
        return result;
    });

    return {
        tokens: completionStream,
        result: completionPromise,
    };
}

async function* createCompletionGenerator(provider, input, options) {
    const completion = createCompletionStream(provider, input, options);
    for await (const chunk of completion.tokens) {
        yield chunk;
    }
    return await completion.result;
}

module.exports = {
    DEFAULT_LIBRARIES_DIRECTORY,
    DEFAULT_DIRECTORY,
    DEFAULT_PROMPT_CONTEXT,
    DEFAULT_MODEL_CONFIG,
    DEFAULT_MODEL_LIST_URL,
    LLModel,
    InferenceModel,
    EmbeddingModel,
    ChatSession,
    createCompletion,
    createCompletionStream,
    createCompletionGenerator,
    createEmbedding,
    downloadModel,
    retrieveModel,
    loadModel,
};
