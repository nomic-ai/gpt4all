'use strict';

/// This file implements the gpt4all.d.ts file endings.
/// Written in commonjs to support both ESM and CJS projects.
const path = require('path');
const { LLModel } = require('node-gyp-build')(path.resolve(__dirname, '..'));
const { createWriteStream, existsSync } = require('fs');
const { performance } = require('node:perf_hooks');
const { resolve } = require('path/posix');


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
    options = { debug: false, location: process.cwd(), url: undefined }
) {
    const abortController = new AbortController();
    const signal = abortController.signal;

    const downloadLocation = options.location ?? process.cwd();
    const pathToModel = path.join(downloadLocation, name);
    if(existsSync(pathToModel)) {
        throw Error("Path to model already exists");
    }

    //wrapper function to get the readable stream from request
    const baseUrl = options.url ?? 'https://gpt4all.io/models'
    const fetcher = (name) => fetch(`${baseUrl}/${name}`, {
        signal,
    })
    .then(res => {
         if(!res.ok) {
            throw Error("Could not find "+ name + " from " + baseUrl)
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

function createPrompt (messages, hasDefaultHeader, hasDefaultFooter) {
    let fullPrompt = "";

    for(const message of messages) {
        if(message.role === 'system') {
            const systemMessage = message.content + "\n";
            fullPrompt+= systemMessage;
        }
    }
    if(hasDefaultHeader) {
        fullPrompt+= `### Instruction: 
        The prompt below is a question to answer, a task to complete, or a conversation 
        to respond to; decide which and write an appropriate response.
        \n### Prompt: 
        `;
    }
    for(const message of messages) {
        if (message.role === "user") {
            const user_message = "\n" + message["content"];
            fullPrompt += user_message;
        }
        if (message["role"] == "assistant") {
            const assistant_message = "\nResponse: " + message["content"];
            fullPrompt += assistant_message;
        }
    }
    if(hasDefaultFooter) {
        fullPrompt += "\n### Response:";
    }

    return fullPrompt;
}




exports.createCompletion = function (
    llmodel,
    messages,
    options = {
       hasDefaultHeader: true,
       hasDefaultFooter: false,
       verbose : true
    },
) {
    //creating the keys to insert into promptMaker.
    const fullPrompt = createPrompt(
        messages,
        options.hasDefaultHeader??true,
        options.hasDefaultFooter
    );
    if(options.verbose) {
        console.log(fullPrompt);
    }
    const promisifiedRawPrompt = new Promise((resolve, rej) => {
        llmodel.raw_prompt(fullPrompt, options, (s) => {
            resolve(s);
        })
    });
    return promisifiedRawPrompt
        .then( response  => {
            return {
                llmodel : llmodel.name(),
                usage : {
                    prompt_tokens: fullPrompt.length,
                    completion_tokens: response.length, //TODO
                    total_tokens: fullPrompt.length + response.length //TODO
                },
                choices: [
                    {
                      "message": {
                        "role": "assistant",
                        "content": response
                        }
                    }
                ]}
        });
}
