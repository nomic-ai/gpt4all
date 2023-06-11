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
const config = require("./config.js");

async function loadModel(modelName, options = {}) {
    const loadOptions = {
        modelPath: config.DEFAULT_DIRECTORY,
        librariesPath: config.DEFAULT_LIBRARIES_DIRECTORY,
        allowDownload: true,
        verbose: true,
        ...options,
    };

    await retrieveModel(modelName, {
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

    const llmOptions = {
        model_name: appendBinSuffixIfMissing(modelName),
        model_path: loadOptions.modelPath,
        library_path: libPath,
    };

    if (loadOptions.verbose) {
        console.log("Creating LLModel with options:", llmOptions);
    }
    const llmodel = new LLModel(llmOptions);

    return llmodel;
}

function createPrompt(messages, hasDefaultHeader, hasDefaultFooter) {
    let fullPrompt = "";

    for (const message of messages) {
        if (message.role === "system") {
            const systemMessage = message.content + "\n";
            fullPrompt += systemMessage;
        }
    }
    if (hasDefaultHeader) {
        fullPrompt += `### Instruction: 
        The prompt below is a question to answer, a task to complete, or a conversation 
        to respond to; decide which and write an appropriate response.
        \n### Prompt: 
        `;
    }
    for (const message of messages) {
        if (message.role === "user") {
            const user_message = "\n" + message["content"];
            fullPrompt += user_message;
        }
        if (message["role"] == "assistant") {
            const assistant_message = "\nResponse: " + message["content"];
            fullPrompt += assistant_message;
        }
    }
    if (hasDefaultFooter) {
        fullPrompt += "\n### Response:";
    }

    return fullPrompt;
}

async function createCompletion(
    llmodel,
    messages,
    options = {
        hasDefaultHeader: true,
        hasDefaultFooter: false,
        verbose: true,
    }
) {
    //creating the keys to insert into promptMaker.
    const fullPrompt = createPrompt(
        messages,
        options.hasDefaultHeader ?? true,
        options.hasDefaultFooter
    );
    if (options.verbose) {
        console.log("Sent: " + fullPrompt);
    }
    const promisifiedRawPrompt = new Promise((resolve, rej) => {
        llmodel.raw_prompt(fullPrompt, options, (s) => {
            resolve(s);
        });
    });
    return promisifiedRawPrompt.then((response) => {
        return {
            llmodel: llmodel.name(),
            usage: {
                prompt_tokens: fullPrompt.length,
                completion_tokens: response.length, //TODO
                total_tokens: fullPrompt.length + response.length, //TODO
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
    });
}

module.exports = {
    ...config,
    LLModel,
    createCompletion,
    downloadModel,
    retrieveModel,
    loadModel,
};
