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
const { InferenceModel, EmbeddingModel } = require('./models.js');


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
        console.debug("Creating LLModel with options:", llmOptions);
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

function formatChatPrompt(messages, { systemPrompt, promptTemplate, promptFooter, promptHeader }) {

    const systemPromptLines = [];
    
    if (systemPrompt) {
        systemPromptLines.push(systemPrompt);
    }
    for (const message of messages) {
        if (message.role === "system") {
            const systemMessage = message.content;
            systemPromptLines.push(systemMessage);
        }
    }
    
    let fullPrompt = ""
    
    if (promptHeader || systemPromptLines.length > 0) {
        if (promptHeader) {
            fullPrompt += promptHeader;
        }
        if (systemPromptLines.length > 0) {
            fullPrompt += systemPromptLines.join('\n');
        }
        
        fullPrompt += '\n\n';
    }

    
    for (const message of messages) {
        if (message.role === "user") {
            const userMessage = promptTemplate.replace('%1', message["content"]);
            fullPrompt += userMessage;
        }
        if (message["role"] == "assistant") {
            const assistantMessage = message["content"] + '\n';
            fullPrompt += assistantMessage;
        }
    }
    
    if (promptFooter) {
        fullPrompt += '\n\n' + promptFooter;
    }
    
    return fullPrompt

}

async function createEmbedding(model, text) {
    return model.embed(text);
}

async function createCompletion(
    model,
    messages,
    options = {
        // systemPrompt: '',
        // promptTemplate: '%1',
        verbose: true,
        max_tokens: 200,
        temp: 0.7,
        top_p: 0.4,
        top_k: 40,
        repeat_penalty: 1.18,
        repeat_last_n: 64,
        n_batch: 8,
    }
) {
    
    const prompt = formatChatPrompt(
        messages,
        {
            systemPrompt: options.systemPrompt || model.config.systemPrompt,
            // systemPromptTemplate: options.systemPromptTemplate || model.config.systemPromptTemplate,
            promptTemplate: options.promptTemplate || model.config.promptTemplate || '%1',
            promptHeader: options.promptHeader || '',
            promptFooter: options.promptFooter || '',
            // promptHeader: options.promptHeader || '### Instruction: The prompt below is a question to answer, a task to complete, or a conversation to respond to; decide which and write an appropriate response.',
           // promptFooter: options.promptFooter || '### Response:',
        }
    );
    
    if (options.verbose) {
        console.log("Sending Prompt:\n" + prompt);
    }
    
    const response = await model.generate(prompt, {
        max_tokens: 200,
        temp: 0.7,
        top_p: 0.4,
        top_k: 40,
        repeat_penalty: 1.18,
        repeat_last_n: 64,
        n_batch: 8,
        ...options,
    });
    
    if (options.verbose) {
        console.log("Received Response:\n"+response);
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
    throw Error("This API has not been completed yet!")
}

module.exports = {
    DEFAULT_LIBRARIES_DIRECTORY,
    DEFAULT_DIRECTORY,
    LLModel,
    InferenceModel,
    EmbeddingModel,
    createCompletion,
    createEmbedding,
    downloadModel,
    retrieveModel,
    loadModel,
    createTokenStream
};
