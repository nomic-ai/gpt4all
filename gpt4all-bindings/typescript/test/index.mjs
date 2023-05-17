import * as assert from 'assert';
import { LLModel } from '../src/gpt4all.js'

const ll = new LLModel("./ggml-vicuna-7b-1.1-q4_2.bin");


console.log("state size " + ll.stateSize())

console.log("thread count " + ll.threadCount());
ll.setThreadCount(5);
console.log("thread count " + ll.threadCount());
ll.setThreadCount(4);
console.log("thread count " + ll.threadCount());
ll.prompt("HEllo")
console.log("ok")
