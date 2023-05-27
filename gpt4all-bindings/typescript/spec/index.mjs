import { LLModel, prompt, createCompletion } from '../src/gpt4all.js'
import { EventEmitter } from 'node:events';
import { abort } from 'process';

const ll = new LLModel("./ggml-vicuna-7b-1.1-q4_2.bin");
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
    prompt`${"header"} ${"prompt"}`, {
        verbose: false,
        prompt: 'hello! Say something thought provoking.'
    }
);

createCompletion(ll, 
    prompt`${"header"}, ${"prompt"}`, {
        verbose: false,
        prompt: 'hello, repeat what was said previously.'
    }
)
