"use strict";

/// This file implements the gpt4all.d.ts file endings.
/// Written in commonjs to support both ESM and CJS projects.
const path = require("node:path");
const os = require("node:os");
const { LLModel } = require("node-gyp-build")(path.resolve(__dirname, ".."));
const util = require("./util.js");
const DEFAULT_DIRECTORY = path.join(os.homedir(), "cache", "gpt4all");

exports.download = util.download;
exports.listModels = util.listModels;

exports.LLModel = LLModel;
exports.DEFAULT_DIRECTORY = DEFAULT_DIRECTORY;

const librarySearchPaths = [
    path.join(DEFAULT_DIRECTORY, "libraries"),
    path.resolve("./libraries"),
    process.cwd(),
];
exports.DEFAULT_LIBRARIES_DIRECTORY = librarySearchPaths.join(";");

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

exports.createCompletion = async function (
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
};
