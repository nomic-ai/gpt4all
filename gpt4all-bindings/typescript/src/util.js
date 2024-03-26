const { createWriteStream, existsSync, statSync, mkdirSync } = require("node:fs");
const fsp = require("node:fs/promises");
const { performance } = require("node:perf_hooks");
const path = require("node:path");
const md5File = require("md5-file");
const {
    DEFAULT_DIRECTORY,
    DEFAULT_MODEL_CONFIG,
    DEFAULT_MODEL_LIST_URL,
} = require("./config.js");

async function listModels(
    options = {
        url: DEFAULT_MODEL_LIST_URL,
    }
) {
    if (!options || (!options.url && !options.file)) {
        throw new Error(
            `No model list source specified. Please specify either a url or a file.`
        );
    }

    if (options.file) {
        if (!existsSync(options.file)) {
            throw new Error(`Model list file ${options.file} does not exist.`);
        }

        const fileContents = await fsp.readFile(options.file, "utf-8");
        const modelList = JSON.parse(fileContents);
        return modelList;
    } else if (options.url) {
        const res = await fetch(options.url);

        if (!res.ok) {
            throw Error(
                `Failed to retrieve model list from ${url} - ${res.status} ${res.statusText}`
            );
        }
        const modelList = await res.json();
        return modelList;
    }
}

function appendBinSuffixIfMissing(name) {
    const ext = path.extname(name);
    if (![".bin", ".gguf"].includes(ext)) {
        return name + ".gguf";
    }
    return name;
}

function prepareMessagesForIngest(messages) {
    const systemMessages = messages.filter(
        (message) => message.role === "system"
    );
    if (systemMessages.length > 0) {
        console.warn(
            "System messages are currently not supported and will be ignored. Use the systemPrompt option instead."
        );
    }

    const userAssistantMessages = messages.filter(
        (message) => message.role !== "system"
    );

    // make sure the first message is a user message
    // if its not, the turns will be out of order
    if (userAssistantMessages[0].role !== "user") {
        userAssistantMessages.unshift({
            role: "user",
            content: "",
        });
    }

    // create turns of user input + assistant reply
    const turns = [];
    let userMessage = null;
    let assistantMessage = null;

    for (const message of userAssistantMessages) {
        // consecutive messages of the same role are concatenated into one message
        if (message.role === "user") {
            if (!userMessage) {
                userMessage = message.content;
            } else {
                userMessage += "\n" + message.content;
            }
        } else if (message.role === "assistant") {
            if (!assistantMessage) {
                assistantMessage = message.content;
            } else {
                assistantMessage += "\n" + message.content;
            }
        }

        if (userMessage && assistantMessage) {
            turns.push({
                user: userMessage,
                assistant: assistantMessage,
            });
            userMessage = null;
            assistantMessage = null;
        }
    }

    return turns;
}

// readChunks() reads from the provided reader and yields the results into an async iterable
// https://css-tricks.com/web-streams-everywhere-and-fetch-for-node-js/
function readChunks(reader) {
    return {
        async *[Symbol.asyncIterator]() {
            let readResult = await reader.read();
            while (!readResult.done) {
                yield readResult.value;
                readResult = await reader.read();
            }
        },
    };
}

function downloadModel(modelName, options = {}) {
    const downloadOptions = {
        modelPath: DEFAULT_DIRECTORY,
        verbose: false,
        ...options,
    };

    const modelFileName = appendBinSuffixIfMissing(modelName);
    const partialModelPath = path.join(
        downloadOptions.modelPath,
        modelName + ".part"
    );
    const finalModelPath = path.join(downloadOptions.modelPath, modelFileName);
    const modelUrl =
        downloadOptions.url ??
        `https://gpt4all.io/models/gguf/${modelFileName}`;

    mkdirSync(downloadOptions.modelPath, { recursive: true });

    if (existsSync(finalModelPath)) {
        throw Error(`Model already exists at ${finalModelPath}`);
    }

    if (downloadOptions.verbose) {
        console.debug(`Downloading ${modelName} from ${modelUrl}`);
    }

    const headers = {
        "Accept-Ranges": "arraybuffer",
        "Response-Type": "arraybuffer",
    };

    const writeStreamOpts = {};

    if (existsSync(partialModelPath)) {
        if (downloadOptions.verbose) {
            console.debug("Partial model exists, resuming download...");
        }
        const startRange = statSync(partialModelPath).size;
        headers["Range"] = `bytes=${startRange}-`;
        writeStreamOpts.flags = "a";
    }

    const abortController = new AbortController();
    const signal = abortController.signal;

    const finalizeDownload = async () => {
        if (downloadOptions.md5sum) {
            const fileHash = await md5File(partialModelPath);
            if (fileHash !== downloadOptions.md5sum) {
                await fsp.unlink(partialModelPath);
                const message = `Model "${modelName}" failed verification: Hashes mismatch. Expected ${downloadOptions.md5sum}, got ${fileHash}`;
                throw Error(message);
            }
            if (downloadOptions.verbose) {
                console.debug(`MD5 hash verified: ${fileHash}`);
            }
        }

        await fsp.rename(partialModelPath, finalModelPath);
    };

    // a promise that executes and writes to a stream. Resolves to the path the model was downloaded to when done writing.
    const downloadPromise = new Promise((resolve, reject) => {
        let timestampStart;

        if (downloadOptions.verbose) {
            console.debug(`Downloading @ ${partialModelPath} ...`);
            timestampStart = performance.now();
        }

        const writeStream = createWriteStream(
            partialModelPath,
            writeStreamOpts
        );

        writeStream.on("error", (e) => {
            writeStream.close();
            reject(e);
        });

        writeStream.on("finish", () => {
            if (downloadOptions.verbose) {
                const elapsed = performance.now() - timestampStart;
                console.log(`Finished. Download took ${elapsed.toFixed(2)} ms`);
            }

            finalizeDownload()
                .then(() => {
                    resolve(finalModelPath);
                })
                .catch(reject);
        });

        fetch(modelUrl, {
            signal,
            headers,
        })
            .then((res) => {
                if (!res.ok) {
                    const message = `Failed to download model from ${modelUrl} - ${res.status} ${res.statusText}`;
                    reject(Error(message));
                }
                return res.body.getReader();
            })
            .then(async (reader) => {
                for await (const chunk of readChunks(reader)) {
                    writeStream.write(chunk);
                }
                writeStream.end();
            })
            .catch(reject);
    });

    return {
        cancel: () => abortController.abort(),
        promise: downloadPromise,
    };
}

async function retrieveModel(modelName, options = {}) {
    const retrieveOptions = {
        modelPath: DEFAULT_DIRECTORY,
        allowDownload: true,
        verbose: false,
        ...options,
    };
    mkdirSync(retrieveOptions.modelPath, { recursive: true });

    const modelFileName = appendBinSuffixIfMissing(modelName);
    const fullModelPath = path.join(retrieveOptions.modelPath, modelFileName);
    const modelExists = existsSync(fullModelPath);

    let config = { ...DEFAULT_MODEL_CONFIG };

    const availableModels = await listModels({
        file: retrieveOptions.modelConfigFile,
        url:
            retrieveOptions.allowDownload &&
            "https://gpt4all.io/models/models3.json",
    });

    const loadedModelConfig = availableModels.find(
        (model) => model.filename === modelFileName
    );

    if (loadedModelConfig) {
        config = {
            ...config,
            ...loadedModelConfig,
        };
    } else {
        // if there's no local modelConfigFile specified, and allowDownload is false, the default model config will be used.
        // warning the user here because the model may not work as expected.
        console.warn(
            `Failed to load model config for ${modelName}. Using defaults.`
        );
    }

    config.systemPrompt = config.systemPrompt.trim();

    if (modelExists) {
        config.path = fullModelPath;

        if (retrieveOptions.verbose) {
            console.debug(`Found ${modelName} at ${fullModelPath}`);
        }
    } else if (retrieveOptions.allowDownload) {
        const downloadController = downloadModel(modelName, {
            modelPath: retrieveOptions.modelPath,
            verbose: retrieveOptions.verbose,
            filesize: config.filesize,
            url: config.url,
            md5sum: config.md5sum,
        });

        const downloadPath = await downloadController.promise;
        config.path = downloadPath;

        if (retrieveOptions.verbose) {
            console.debug(`Model downloaded to ${downloadPath}`);
        }
    } else {
        throw Error("Failed to retrieve model.");
    }
    return config;
}

module.exports = {
    appendBinSuffixIfMissing,
    prepareMessagesForIngest,
    downloadModel,
    retrieveModel,
    listModels,
};
