const { createWriteStream, existsSync, unlink } = require("node:fs");
const { performance } = require("node:perf_hooks");
const path = require("node:path");
const {mkdirp} = require("mkdirp");
const { DEFAULT_DIRECTORY, DEFAULT_LIBRARIES_DIRECTORY } = require("./config.js");
const md5File = require('md5-file');
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
        debug: false,
        url: "https://gpt4all.io/models",
        ...options,
    };

    const modelFileName = appendBinSuffixIfMissing(modelName);
    const partialModelPath = path.join(
        downloadOptions.modelPath,
        modelName + ".part"
    );
    const finalModelPath = path.join(downloadOptions.modelPath, modelFileName);
    const modelUrl = `${downloadOptions.url}/${modelFileName}`;

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

    // wrapper function to get the readable stream from request
    const fetchModel = (fetchOpts = {}) =>
        fetch(modelUrl, {
            signal,
            ...fetchOpts,
        }).then((res) => {
            if (!res.ok) {
                throw Error(
                    `Failed to download model from ${modelUrl} - ${res.statusText}`
                );
            }
            return res.body.getReader();
        });

    // a promise that executes and writes to a stream. Resolves when done writing.
    const res = new Promise((resolve, reject) => {
        fetchModel({ headers })
            // Resolves an array of a reader and writestream.
            .then((reader) => [
                reader,
                createWriteStream(partialModelPath, writeStreamOpts),
            ])
            .then(async ([readable, wstream]) => {
                console.log("Downloading @ ", partialModelPath);
                let perf;

                if (options.debug) {
                    perf = performance.now();
                }

                wstream.on("finish", () => {
                    if (options.debug) {
                        console.log(
                            "Time taken: ",
                            (performance.now() - perf).toFixed(2),
                            " ms"
                        );
                    }
                    wstream.close();
                });

                wstream.on("error", (e) => {
                    wstream.close();
                    reject(e);
                });

                for await (const chunk of readChunks(readable)) {
                    wstream.write(chunk);
                }

                if (options.md5sum) {
                    const fileHash = await md5File(partialModelPath);
                    if (fileHash !== options.md5sum) {
                        await fs.unlink(partialModelPath);
                        return reject(
                            Error(`Model "${modelName}" failed verification: Hashes mismatch`)
                        );
                    }
                    if (options.debug) {
                        console.log("MD5 hash verified: ", fileHash);
                    }
                }

                await fs.rename(partialModelPath, finalModelPath);
                resolve(finalModelPath);
            })
            .catch(reject);
    });

    return {
        cancel: () => abortController.abort(),
        promise: () => res,
    };
}

async function retrieveModel (
    modelName,
    options = {}
) {
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

    if (modelExists) {
        return fullModelPath;
    }

    if (!retrieveOptions.allowDownload) {
        throw Error(`Model does not exist at ${fullModelPath}`);
    }

    const availableModels = await listModels();
    const foundModel = availableModels.find((model) => model.filename === modelFileName);

    if (!foundModel) {
        throw Error(`Model "${modelName}" is not available.`);
    }

    if (retrieveOptions.verbose) {
        console.log(`Downloading ${modelName}...`);
    }

    const downloadController = downloadModel(modelName, {
        modelPath: retrieveOptions.modelPath,
        debug: retrieveOptions.verbose,
    });

    const downloadPath = await downloadController.promise();

    if (retrieveOptions.verbose) {
        console.log(`Model downloaded to ${downloadPath}`);
    }

    return downloadPath

}


module.exports = {
    appendBinSuffixIfMissing,
    downloadModel,
    retrieveModel,
    listModels
};
