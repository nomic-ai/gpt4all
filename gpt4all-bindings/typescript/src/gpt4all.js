/// This file implements the gpt4all.d.ts file endings.
/// Written in commonjs to support both ESM and CJS projects.

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


exports.download = function (
    name,
    options = { debug: false, location: process.cwd(), link: undefined }
) {
    const abortController = new AbortController();
    const signal = abortController.signal;

    const pathToModel = join(options.location, name);
    if(existsSync(pathToModel)) {
        throw Error("Path to model already exists");
    }

    //wrapper function to get the readable stream from request
    const fetcher = (name) => fetch(options.link ?? `https://gpt4all.io/models/${name}`, {
        signal,
    })
    .then(res => {
         if(!res.ok) {
            throw Error("Could not find "+ name + " from " + `https://gpt4all.io/models/` )
         }
         return res.body.getReader()
    })
    
    //a promise that executes and writes to a stream. Resolves when done writing.
    const res = new Promise((resolve, reject) => {
        fetcher(name)
            //Resolves an array of a reader and writestream.
            .then(reader => [reader, createWriteStream(pathToModel)])
            .then( 
            async ([readable, wstream]) => {
                console.log('(CLI might hang) Downloading @ ', pathToModel);
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


//https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals#tagged_templates
exports.prompt = function prompt(strings, ...keys) {
  return (...values) => {
    const dict = values[values.length - 1] || {};
    const result = [strings[0]];
    keys.forEach((key, i) => {
      const value = Number.isInteger(key) ? values[key] : dict[key];
      result.push(value, strings[i + 1]);
    });
    return result.join("");
  };
}



exports.createCompletion = function (llmodel, promptMaker, options) {
    //creating the keys to insert into promptMaker.
    const entries = { 
        system: options.system ?? '',
        header: options.header ?? "### Instruction: The prompt below is a question to answer, a task to complete, or a conversation to respond to; decide which and write an appropriate response.\n### Prompt: ",
        prompt: options.prompt,
        ...(options.promptEntries ?? {})
    };
    
    const fullPrompt = promptMaker(entries)+'\n### Response:';

    if(options.verbose) {
       console.log("sending prompt: " + `"${fullPrompt}"`) 
    }
    
    return llmodel.raw_prompt(fullPrompt, options);
}
