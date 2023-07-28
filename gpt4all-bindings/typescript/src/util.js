const { createWriteStream, existsSync, statSync } = require("node:fs");
const fsp = require("node:fs/promises");
const { performance } = require("node:perf_hooks");
const path = require("node:path");
const { mkdirp } = require("mkdirp");
const md5File = require("md5-file");
const { DEFAULT_DIRECTORY, DEFAULT_MODEL_CONFIG } = require("./config.js");

async function listModels() {
    const res = await fetch("https://gpt4all.io/models/models.json");
    const modelList = await res.json();
    return modelList;
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

function downloadModel(modelName, options = {}) {
    const downloadOptions = {
        modelPath: DEFAULT_DIRECTORY,
        verbose: false,
        md5sum: true,
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

    if (existsSync(finalModelPath)) {
        throw Error(`Model already exists at ${finalModelPath}`);
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

    // a promise that executes and writes to a stream. Resolves to the path the model was downloaded to when done writing.
    const downloadPromise = new Promise((resolve, reject) => {
        const writeStream = createWriteStream(
            partialModelPath,
            writeStreamOpts
        );

        fetch(modelUrl, {
            signal,
            headers,
        })
            .then((res) => {
                if (!res.ok) {
                    reject(
                        Error(
                            `Failed to download model from ${modelUrl} - ${res.statusText}`
                        )
                    );
                }
                return res.body.getReader();
            })
            .then(async (reader) => {
                console.log("Downloading @ ", partialModelPath);
                let perf;

                if (options.verbose) {
                    perf = performance.now();
                }

                writeStream.on("finish", () => {
                    if (options.verbose) {
                        console.log(
                            "Time taken: ",
                            (performance.now() - perf).toFixed(2),
                            " ms"
                        );
                    }
                    writeStream.close();
                });

                writeStream.on("error", (e) => {
                    writeStream.close();
                    reject(e);
                });

                for await (const chunk of readChunks(reader)) {
                    writeStream.write(chunk);
                }

                if (options.md5sum) {
                    const fileHash = await md5File(partialModelPath);
                    if (fileHash !== options.md5sum) {
                        await fsp.unlink(partialModelPath);
                        return reject(
                            Error(
                                `Model "${modelName}" failed verification: Hashes mismatch`
                            )
                        );
                    }
                    if (options.verbose) {
                        console.log("MD5 hash verified: ", fileHash);
                    }
                }

                await fsp.rename(partialModelPath, finalModelPath);
                resolve(finalModelPath);
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

    if (retrieveOptions.allowDownload) {
        const availableModels = await listModels();
        const downloadedConfig = availableModels.find(
            (model) => model.filename === modelFileName
        );
        if (downloadedConfig) {
            config = {
                ...config,
                ...downloadedConfig,
            };
        }
    }
    
    config.systemPrompt = config.systemPrompt.trim();
    // config.promptTemplate = config.promptTemplate;

    if (modelExists) {
        config.path = fullModelPath;

        if (retrieveOptions.verbose) {
            console.log(`Found ${modelName} at ${fullModelPath}`);
        }
    } else if (retrieveOptions.allowDownload) {
        if (retrieveOptions.verbose) {
            console.log(`Downloading ${modelName} from ${config.url} ...`);
        }

        const downloadController = downloadModel(modelName, {
            modelPath: retrieveOptions.modelPath,
            verbose: retrieveOptions.verbose,
            url: config.url,
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
};
