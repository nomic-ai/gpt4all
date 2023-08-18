const { createWriteStream, existsSync, statSync } = require("node:fs");
const fsp = require("node:fs/promises");
const { performance } = require("node:perf_hooks");
const path = require("node:path");
const { mkdirp } = require("mkdirp");
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
    if (!name.endsWith(".bin")) {
        return name + ".bin";
    }
    return name;
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

/**
 * Prints a warning if any keys in the prompt context are snake_case.
 */
function warnOnSnakeCaseKeys(promptContext) {
    const snakeCaseKeys = Object.keys(promptContext).filter((key) =>
        key.includes("_")
    );

    if (snakeCaseKeys.length > 0) {
        console.warn(
            "Prompt context keys should be camelCase. Support for snake_case might be removed in the future. Found keys: " +
                snakeCaseKeys.join(", ")
        );
    }
}

/**
 * Converts all keys in the prompt context to snake_case
 * For duplicate definitions, the value of the last occurrence will be used.
 */
function normalizePromptContext(promptContext) {
    const normalizedPromptContext = {};

    for (const key in promptContext) {
        if (promptContext.hasOwnProperty(key)) {
            const snakeKey = key.replace(
                /[A-Z]/g,
                (match) => `_${match.toLowerCase()}`
            );
            normalizedPromptContext[snakeKey] = promptContext[key];
        }
    }

    return normalizedPromptContext;
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
        downloadOptions.url ?? `https://gpt4all.io/models/${modelFileName}`;

    mkdirp.sync(downloadOptions.modelPath)

    if (existsSync(finalModelPath)) {
        throw Error(`Model already exists at ${finalModelPath}`);
    }
    
    if (downloadOptions.verbose) {
        console.log(`Downloading ${modelName} from ${modelUrl}`);
    }

    const headers = {
        "Accept-Ranges": "arraybuffer",
        "Response-Type": "arraybuffer",
    };

    const writeStreamOpts = {};

    if (existsSync(partialModelPath)) {
        console.log("Partial model exists, resuming download...");
        const startRange = statSync(partialModelPath).size;
        headers["Range"] = `bytes=${startRange}-`;
        writeStreamOpts.flags = "a";
    }

    const abortController = new AbortController();
    const signal = abortController.signal;

    const finalizeDownload = async () => {
        if (options.md5sum) {
            const fileHash = await md5File(partialModelPath);
            if (fileHash !== options.md5sum) {
                await fsp.unlink(partialModelPath);
                const message = `Model "${modelName}" failed verification: Hashes mismatch. Expected ${options.md5sum}, got ${fileHash}`;
                throw Error(message);
            }
            if (options.verbose) {
                console.log(`MD5 hash verified: ${fileHash}`);
            }
        }

        await fsp.rename(partialModelPath, finalModelPath);
    };

    // a promise that executes and writes to a stream. Resolves to the path the model was downloaded to when done writing.
    const downloadPromise = new Promise((resolve, reject) => {
        let timestampStart;

        if (options.verbose) {
            console.log(`Downloading @ ${partialModelPath} ...`);
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
            if (options.verbose) {
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
        verbose: true,
        ...options,
    };

    await mkdirp(retrieveOptions.modelPath);

    const modelFileName = appendBinSuffixIfMissing(modelName);
    const fullModelPath = path.join(retrieveOptions.modelPath, modelFileName);
    const modelExists = existsSync(fullModelPath);

    let config = { ...DEFAULT_MODEL_CONFIG };

    const availableModels = await listModels({
        file: retrieveOptions.modelConfigFile,
        url:
            retrieveOptions.allowDownload &&
            "https://gpt4all.io/models/models.json",
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
            console.log(`Found ${modelName} at ${fullModelPath}`);
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
            console.log(`Model downloaded to ${downloadPath}`);
        }
    } else {
        throw Error("Failed to retrieve model.");
    }

    return config;
}

module.exports = {
    appendBinSuffixIfMissing,
    downloadModel,
    retrieveModel,
    listModels,
    normalizePromptContext,
    warnOnSnakeCaseKeys,
};
