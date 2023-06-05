import { LLModel, createCompletion } from '../src/gpt4all.js'
const ll= new LLModel("./ggml-vicuna-7b-1.1-q4_2.bin");

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


createCompletion(
    ll,
    [
        { role : 'system', content: 'be as annoying as possible'  },
        { role : 'user', content: 'Consider the most annoying quote and say it in the most annoying way'  }, 
    ],
    { verbose: true }
);
console.log('new prompt');

//console.log(createCompletion(
//    ll,
//    prompt`${"header"} ${"prompt"}`, {
//        verbose: false,
//        prompt: 'hello! Repeat what you previously said.'
//    }
//))


