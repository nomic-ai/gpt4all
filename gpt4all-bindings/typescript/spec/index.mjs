import { LLModel, createCompletion } from '../src/gpt4all.js'
const ll = new LLModel('./ggml-gpt4all-j-v1.3-groovy.bin');

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


