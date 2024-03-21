import { loadModel, createCompletion } from "../src/gpt4all.js";

const model = await loadModel("orca-mini-3b-gguf2-q4_0.gguf", {
    verbose: true,
    device: "gpu",
});

const messages = [
    {
        role: "system",
        content: "<|im_start|>system\nYou are an advanced mathematician.\n<|im_end|>\n",
    },
    {
        role: "user",
        content: "What's 2+2?",
    },
    {
        role: "assistant",
        content: "5",
    },
    {
        role: "user",
        content: "Are you sure?",
    },
];


const res1 = await createCompletion(model, messages);
console.debug(res1.choices[0].message);
messages.push(res1.choices[0].message);

messages.push({
    role: "user",
    content: "Could you double check that?",
});

const res2 = await createCompletion(model, messages);
console.debug(res2.choices[0].message);
messages.push(res2.choices[0].message);

messages.push({
    role: "user",
    content: "Let's bring out the big calculators.",
});

const res3 = await createCompletion(model, messages);
console.debug(res3.choices[0].message);
messages.push(res3.choices[0].message);

// console.debug(messages);
