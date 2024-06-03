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
        nCtx: 2048,
        nGpuLayers: options.ngl ?? 100,
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
    const existingLibPaths = loadOptions.librariesPath
        .split(";")
        .filter(existsSync)
        .join(";");
        
    const llmOptions = {
        modelFile: modelConfig.path,
        librariesPath: existingLibPaths,
        nCtx: loadOptions.nCtx,
        nGpuLayers: loadOptions.nGpuLayers,
    };

    let initDevice;
    if (process.platform === "darwin") {
        if (!loadOptions.device) {
            llmOptions.backend = "auto"; // 'auto' is effectively 'metal' due to currently non-functional fallback
        } else if (loadOptions.device === "cpu") {
            llmOptions.backend = "cpu";
        } else {
            if (process.arch !== "arm64" || loadOptions.device !== "gpu") {
                throw new Error(
                    `Unknown device for this platform: ${loadOptions.device}`
                );
            }
            llmOptions.backend = "metal";
        }
    } else {
        // default to kompute. use cpu for arm64 because we currently dont build kompute runtimes for arm64
        llmOptions.backend = process.arch === "arm64" ? "cpu" : "kompute";
        if (!loadOptions.device || loadOptions.device === "cpu") {
            // use the default backend
        } else if (
            loadOptions.device === "cuda" ||
            loadOptions.device === "kompute"
        ) {
            llmOptions.backend = loadOptions.device;
            initDevice = "gpu";
        } else if (loadOptions.device.startsWith("cuda:")) {
            llmOptions.backend = "cuda";
            initDevice = loadOptions.device.replace(/^cuda:/, "");
        } else {
            initDevice = loadOptions.device.replace(/^kompute:/, "");
        }
    }

    if (loadOptions.verbose) {
        console.debug("Creating LLModel:", {
            initDevice,
            llmOptions,
            modelConfig,
        });
    }
    const llmodel = new LLModel(llmOptions);
    if (initDevice) {
        const gpuInitSuccess = llmodel.initGpu(initDevice);
        if (!gpuInitSuccess) {
            const availableDevices = llmodel.getGpuDevices();
            const deviceNames = availableDevices
                .map((device) => device.name)
                .join(", ");
            console.warn(
                `Failed to initialize GPU device "${initDevice}" - Available devices: ${deviceNames}`
            );
        }
    }
    llmodel.load();
    
    if (loadOptions.nThreads) {
        llmodel.setThreadCount(loadOptions.nThreads);
    }

    if (loadOptions.type === "embedding") {
        return new EmbeddingModel(llmodel, modelConfig);
    } else if (loadOptions.type === "inference") {
        return new InferenceModel(llmodel, modelConfig);
    } else {
        throw Error("Invalid model type: " + loadOptions.type);
    }
}

function createEmbedding(model, text, options = {}) {
    let {
        dimensionality = undefined,
        longTextMode = "mean",
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

    let doMean;
    switch (longTextMode) {
        case "mean":
            doMean = true;
            break;
        case "truncate":
            doMean = false;
            break;
        default:
            throw new Error(
                `Long text mode must be one of 'mean' or 'truncate', got ${longTextMode}`
            );
    }

    return model.embed(text, options?.prefix, dimensionality, doMean, atlas);
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

    const result = await provider.generate(input, completionOptions);

    return {
        model: provider.modelName,
        usage: {
            prompt_tokens: result.tokensIngested,
            total_tokens: result.tokensIngested + result.tokensGenerated,
            completion_tokens: result.tokensGenerated,
            n_past_tokens: result.nPast,
        },
        choices: [
            {
                message: {
                    role: "assistant",
                    content: result.text,
                },
                // TODO some completion APIs also provide logprobs and finish_reason, could look into adding those
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
        onResponseTokens: (tokens) => {
            completionStream.push(tokens.text);
            if (options.onResponseTokens) {
                return options.onResponseTokens(tokens);
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
