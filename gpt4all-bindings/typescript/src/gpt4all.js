"use strict";

/// This file implements the gpt4all.d.ts file endings.
/// Written in commonjs to support both ESM and CJS projects.
const { existsSync } = require("fs");
const path = require("node:path");
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
const assert = require("assert");

/**
 * Loads a machine learning model with the specified name. The defacto way to create a model.
 * By default this will download a model from the official GPT4ALL website, if a model is not present at given path.
 *
 * @param {string} modelName - The name of the model to load.
 * @param {LoadModelOptions|undefined} [options] - (Optional) Additional options for loading the model.
 * @returns {Promise<InferenceModel | EmbeddingModel>} A promise that resolves to an instance of the loaded LLModel.
 */
async function loadModel(modelName, options = {}) {
    const loadOptions = {
        modelPath: DEFAULT_DIRECTORY,
        librariesPath: DEFAULT_LIBRARIES_DIRECTORY,
        type: "inference",
        allowDownload: true,
        verbose: true,
        device: 'cpu',
        ...options,
    };

    const modelConfig = await retrieveModel(modelName, {
        modelPath: loadOptions.modelPath,
        modelConfigFile: loadOptions.modelConfigFile,
        allowDownload: loadOptions.allowDownload,
        verbose: loadOptions.verbose,
    });

    assert.ok(typeof loadOptions.librariesPath === 'string');
    const existingPaths = loadOptions.librariesPath
        .split(";")
        .filter(existsSync)
        .join(';');
    console.log("Passing these paths into runtime library search:", existingPaths)

    const llmOptions = {
        model_name: appendBinSuffixIfMissing(modelName),
        model_path: loadOptions.modelPath,
        library_path: existingPaths,
        device: loadOptions.device,
    };

    if (loadOptions.verbose) {
        console.debug("Creating LLModel with options:", llmOptions);
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

/**
 * Formats a list of messages into a single prompt string.
 */
function formatChatPrompt(
    messages,
    {
        systemPromptTemplate,
        defaultSystemPrompt,
        promptTemplate,
        promptFooter,
        promptHeader,
    }
) {
    const systemMessages = messages
        .filter((message) => message.role === "system")
        .map((message) => message.content);

    let fullPrompt = "";

    if (promptHeader) {
        fullPrompt += promptHeader + "\n\n";
    }

    if (systemPromptTemplate) {
        // if user specified a template for the system prompt, put all system messages in the template
        let systemPrompt = "";

        if (systemMessages.length > 0) {
            systemPrompt += systemMessages.join("\n");
        }

        if (systemPrompt) {
            fullPrompt +=
                systemPromptTemplate.replace("%1", systemPrompt) + "\n";
        }
    } else if (defaultSystemPrompt) {
        // otherwise, use the system prompt from the model config and ignore system messages
        fullPrompt += defaultSystemPrompt + "\n\n";
    }

    if (systemMessages.length > 0 && !systemPromptTemplate) {
        console.warn(
            "System messages were provided, but no systemPromptTemplate was specified. System messages will be ignored."
        );
    }

    for (const message of messages) {
        if (message.role === "user") {
            const userMessage = promptTemplate.replace(
                "%1",
                message["content"]
            );
            fullPrompt += userMessage;
        }
        if (message["role"] == "assistant") {
            const assistantMessage = message["content"] + "\n";
            fullPrompt += assistantMessage;
        }
    }

    if (promptFooter) {
        fullPrompt += "\n\n" + promptFooter;
    }

    return fullPrompt;
}

function createEmbedding(model, text) {
    return model.embed(text);
}

const defaultCompletionOptions = {
    verbose: false,
    ...DEFAULT_PROMPT_CONTEXT,
};

async function createCompletion(
    model,
    messages,
    options = defaultCompletionOptions
) {
    if (options.hasDefaultHeader !== undefined) {
        console.warn(
            "hasDefaultHeader (bool) is deprecated and has no effect, use promptHeader (string) instead"
        );
    }

    if (options.hasDefaultFooter !== undefined) {
        console.warn(
            "hasDefaultFooter (bool) is deprecated and has no effect, use promptFooter (string) instead"
        );
    }

    const optionsWithDefaults = {
        ...defaultCompletionOptions,
        ...options,
    };

    const {
        verbose,
        systemPromptTemplate,
        promptTemplate,
        promptHeader,
        promptFooter,
        ...promptContext
    } = optionsWithDefaults;

    const prompt = formatChatPrompt(messages, {
        systemPromptTemplate,
        defaultSystemPrompt: model.config.systemPrompt,
        promptTemplate: promptTemplate || model.config.promptTemplate || "%1",
        promptHeader: promptHeader || "",
        promptFooter: promptFooter || "",
        // These were the default header/footer prompts used for non-chat single turn completions.
        // both seem to be working well still with some models, so keeping them here for reference.
        // promptHeader: '### Instruction: The prompt below is a question to answer, a task to complete, or a conversation to respond to; decide which and write an appropriate response.',
        // promptFooter: '### Response:',
    });

    if (verbose) {
        console.debug("Sending Prompt:\n" + prompt);
    }

    const response = await model.generate(prompt, promptContext);

    if (verbose) {
        console.debug("Received Response:\n" + response);
    }

    return {
        llmodel: model.llm.name(),
        usage: {
            prompt_tokens: prompt.length,
            completion_tokens: response.length, //TODO
            total_tokens: prompt.length + response.length, //TODO
        },
        choices: [
            {
                message: {
                    role: "assistant",
                    content: response,
                },
            },
        ],
    };
}

function createTokenStream() {
    throw Error("This API has not been completed yet!");
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
    createCompletion,
    createEmbedding,
    downloadModel,
    retrieveModel,
    loadModel,
    createTokenStream,
};
