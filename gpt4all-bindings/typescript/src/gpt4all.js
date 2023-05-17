const { LLModel } = require('bindings')('../build/Release/gpt4allts');
const { createWriteStream, existsSync } = require('fs');
const { join } = require('path');
const { performance } = require('node:perf_hooks');
// readChunks() reads from the provided reader and yields the results into an async iterable
// https://css-tricks.com/web-streams-everywhere-and-fetch-for-node-js/
function readChunks(reader) {
    return {
        async* [Symbol.asyncIterator]() {
            let readResult = await reader.read();
            while (!readResult.done) {
                yield readResult.value;
                readResult = await reader.read();
            }
        },
    };
}

exports.LLModel = LLModel;
exports.download = function (name, options = { debug: false, location: process.cwd() }) {
    const abortController = new AbortController();
    const signal = abortController.signal;


    //wrapper function to get the readable stream from request
    const fetcher = (name) => fetch(`https://gpt4all.io/models/${name}`, 
        {
            signal,
        }
    )
    .then(res => res.body.getReader());

    const pathToModel = join(options.location, name);
    if(existsSync(pathToModel)) {
        throw Error("Path to model already exists");
    }
    const wstream = createWriteStream(pathToModel);
    //a promise that executes and writes to a stream. Resolves when done writing.
    const res = new Promise((resolve, reject) => {
        fetcher(name).then( 
            async readable => {
                console.log('(CLI might hang) Downloading @ ', join(location,name));
                let perf;
                if(options.debug) {
                   perf = performance.now(); 
                }
                for await (const chunk of readChunks(readable)) {
                    wstream.write(chunk);
                }
                if(options.debug) {
                    console.log("Time taken: ", (performance.now()-perf).toFixed(2), " ms"); 
                }
                resolve();
            }
        ).catch(reject);
    });
    
    return {
        cancel : () => abortController.abort(),
        promise: () => res
    }
}
