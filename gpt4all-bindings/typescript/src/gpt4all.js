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
const { DEFAULT_DIRECTORY, DEFAULT_LIBRARIES_DIRECTORY } = require("./config.js");
const { InferenceModel } = require('./InferenceModel.js');
// const { EmbeddingModel } = require('./EmbeddingModel.js');

async function loadModel(modelName, options = {}) {
    const loadOptions = {
        modelPath: DEFAULT_DIRECTORY,
        librariesPath: DEFAULT_LIBRARIES_DIRECTORY,
        type: 'inference',
        allowDownload: true,
        verbose: true,
        ...options,
    };

    const modelConfig = await retrieveModel(modelName, {
        modelPath: loadOptions.modelPath,
        allowDownload: loadOptions.allowDownload,
        verbose: loadOptions.verbose,
    });

    const libSearchPaths = loadOptions.librariesPath.split(";");

    let libPath = null;

    for (const searchPath of libSearchPaths) {
        if (existsSync(searchPath)) {
            libPath = searchPath;
            break;
        }
    }
    if(!libPath) {
        throw Error("Could not find a valid path from " + libSearchPaths);
    }
    const llmOptions = {
        model_name: appendBinSuffixIfMissing(modelName),
        model_path: loadOptions.modelPath,
        library_path: libPath,
    };

    if (loadOptions.verbose) {
        console.log("Creating LLModel with options:", llmOptions);
    }
    const llmodel = new LLModel(llmOptions);

    if (loadOptions.type === 'embedding') {
        return new EmbeddingModel(llmodel, modelConfig);
    } else if (loadOptions.type === 'inference') {
        return new InferenceModel(llmodel, modelConfig);
    } else {
        throw Error("Invalid model type: " + loadOptions.type);
    }

}

function formatChatPrompt(messages, systemPrompt = '', promptTemplate = '{0}') {
    let fullPrompt = [];

    // for (const message of messages) {
    //     if (message.role === "system") {
    //         const systemMessage = message.content;
    //         fullPrompt.push(systemMessage);
    //     }
    // }
    if (systemPrompt) {
        // fullPrompt.push(`### Instruction: The prompt below is a question to answer, a task to complete, or a conversation to respond to; decide which and write an appropriate response.`);
        fullPrompt.push(systemPrompt + '\n');
    }
    
    for (const message of messages) {
        if (message.role === "user") {
            const userMessage = promptTemplate.replace('{0}', message["content"]);
            fullPrompt.push(userMessage);
        }
        if (message["role"] == "assistant") {
            // const assistantMessage = "### Response: " + message["content"];
            const assistantMessage = message["content"];
            fullPrompt.push(assistantMessage);
        }
    }
    // if (hasDefaultFooter) {
    //     fullPrompt.push("### Response:");
    // }

    return fullPrompt.join('\n');

}

function createEmbedding(llmodel, text) {
    return llmodel.embed(text)
}

async function createCompletion(
    model,
    messages,
    options = {
        // systemPrompt: '',
        // promptTemplate: '{0}',
        verbose: true,
    }
) {
    
    const formattedPrompt = formatChatPrompt(
        messages,
        options.systemPrompt || model.config.systemPrompt,
        options.promptTemplate || model.config.promptTemplate,
    );
    
    if (options.verbose) {
        console.log("Sending Prompt: " + formattedPrompt);
    }
    
    const response = await model.generate(formattedPrompt, options);
    
    return {
        llmodel: model.llmodel.name(),
        usage: {
            prompt_tokens: formattedPrompt.length,
            completion_tokens: response.length, //TODO
            total_tokens: formattedPrompt.length + response.length, //TODO
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
    
    // const promisifiedRawPrompt = model.llmodel.raw_prompt(fullPrompt, options, (s) => {});
    // return promisifiedRawPrompt.then((response) => {
    //     return {
    //         llmodel: llmodel.name(),
    //         usage: {
    //             prompt_tokens: fullPrompt.length,
    //             completion_tokens: response.length, //TODO
    //             total_tokens: fullPrompt.length + response.length, //TODO
    //         },
    //         choices: [
    //             {
    //                 message: {
    //                     role: "assistant",
    //                     content: response,
    //                 },
    //             },
    //         ],
    //     };
    // });
}

function createTokenStream() {
    throw Error("This API has not been completed yet!")
}

module.exports = {
    DEFAULT_LIBRARIES_DIRECTORY,
    DEFAULT_DIRECTORY,
    LLModel,
    createCompletion,
    createEmbedding,
    downloadModel,
    retrieveModel,
    loadModel,
    createTokenStream
};
