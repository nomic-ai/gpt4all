import { LLModel, createCompletion, DEFAULT_DIRECTORY, DEFAULT_LIBRARIES_DIRECTORY } from '../src/gpt4all.js'

const ll = new LLModel({
    model_name: 'ggml-vicuna-7b-1.1-q4_2.bin',
    model_path: './', 
    library_path: DEFAULT_LIBRARIES_DIRECTORY
});

try {
   class Extended extends LLModel {
   }

} catch(e) {
    console.log("Extending from native class gone wrong " + e)
}

console.log("state size " + ll.stateSize())

console.log("thread count " + ll.threadCount());
ll.setThreadCount(5);
console.log("thread count " + ll.threadCount());
ll.setThreadCount(4);
console.log("thread count " + ll.threadCount());
console.log("name " + ll.name());
console.log("type: " + ll.type());
console.log("Default directory for models", DEFAULT_DIRECTORY);
console.log("Default directory for libraries", DEFAULT_LIBRARIES_DIRECTORY);

console.log(await createCompletion(
    ll,
    [
        { role : 'system', content: 'You are a girl who likes playing league of legends.'  },
        { role : 'user', content: 'What is the best top laner to play right now?'  }, 
    ],
    { verbose: false}
));


console.log(await createCompletion(
    ll,
    [
        { role : 'user', content: 'What is the best bottom laner to play right now?'  }, 
    ],
))


