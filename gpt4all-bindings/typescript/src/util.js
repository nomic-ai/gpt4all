const { createWriteStream, existsSync } = require("fs");
const { performance } = require("node:perf_hooks");

exports.listModels = async function listModels() {
    const res = await fetch("https://gpt4all.io/models/models.json");
    const rawModelList = await res.json();
    return rawModelList;
};

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

exports.download = function (
    name,
    options = { debug: false, location: process.cwd(), url: undefined }
) {
    const abortController = new AbortController();
    const signal = abortController.signal;

    const downloadLocation = options.location ?? process.cwd();
    const pathToModel = path.join(downloadLocation, name);
    if (existsSync(pathToModel)) {
        throw Error("Path to model already exists");
    }

    //wrapper function to get the readable stream from request
    const baseUrl = options.url ?? "https://gpt4all.io/models";
    const fetcher = (name) =>
        fetch(`${baseUrl}/${name}`, {
            signal,
        }).then((res) => {
            if (!res.ok) {
                throw Error("Could not find " + name + " from " + baseUrl);
            }
            return res.body.getReader();
        });

    //a promise that executes and writes to a stream. Resolves when done writing.
    const res = new Promise((resolve, reject) => {
        fetcher(name)
            //Resolves an array of a reader and writestream.
            .then((reader) => [reader, createWriteStream(pathToModel)])
            .then(async ([readable, wstream]) => {
                console.log("(CLI might hang) Downloading @ ", pathToModel);
                let perf;
                if (options.debug) {
                    perf = performance.now();
                }
                for await (const chunk of readChunks(readable)) {
                    wstream.write(chunk);
                }
                if (options.debug) {
                    console.log(
                        "Time taken: ",
                        (performance.now() - perf).toFixed(2),
                        " ms"
                    );
                }
                resolve();
            })
            .catch(reject);
    });

    return {
        cancel: () => abortController.abort(),
        promise: () => res,
    };
};
