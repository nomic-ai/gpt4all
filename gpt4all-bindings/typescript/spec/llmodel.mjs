import {
    LLModel,
    createCompletion,
    DEFAULT_DIRECTORY,
    DEFAULT_LIBRARIES_DIRECTORY,
    loadModel,
} from "../src/gpt4all.js";

const model = await loadModel("mistral-7b-openorca.gguf2.Q4_0.gguf", {
    verbose: true,
    device: "gpu",
});
const ll = model.llm;

try {
    class Extended extends LLModel {}
} catch (e) {
    console.log("Extending from native class gone wrong " + e);
}

console.log("state size " + ll.stateSize());

console.log("thread count " + ll.threadCount());
ll.setThreadCount(5);

console.log("thread count " + ll.threadCount());
ll.setThreadCount(4);
console.log("thread count " + ll.threadCount());
console.log("name " + ll.name());
console.log("type: " + ll.type());
console.log("Default directory for models", DEFAULT_DIRECTORY);
console.log("Default directory for libraries", DEFAULT_LIBRARIES_DIRECTORY);
console.log("Has GPU", ll.hasGpuDevice());
console.log("gpu devices", ll.listGpu());
console.log("Required Mem in bytes", ll.memoryNeeded());

// to ingest a custom system prompt without using a chat session.
await createCompletion(
    model,
    "<|im_start|>system\nYou are an advanced mathematician.\n<|im_end|>\n",
    {
        promptTemplate: "%1",
        nPredict: 0,
        special: true,
    }
);
const completion1 = await createCompletion(model, "What is 1 + 1?", {
    verbose: true,
});
console.log(`🤖 > ${completion1.choices[0].message.content}`);
//Very specific:
// tested on Ubuntu 22.0, Linux Mint, if I set nPast to 100, the app hangs.
const completion2 = await createCompletion(model, "And if we add two?", {
    verbose: true,
});
console.log(`🤖 > ${completion2.choices[0].message.content}`);

//CALLING DISPOSE WILL INVALID THE NATIVE MODEL. USE THIS TO CLEANUP
model.dispose();

console.log("model disposed, exiting...");
