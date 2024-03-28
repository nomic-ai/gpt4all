import {
    loadModel,
    createCompletion,
} from "../src/gpt4all.js";

const modelOptions = {
    verbose: true,
};

const model1 = await loadModel("orca-mini-3b-gguf2-q4_0.gguf", {
    ...modelOptions,
    device: "gpu", // only one model can be on gpu
});
const model2 = await loadModel("orca-mini-3b-gguf2-q4_0.gguf", modelOptions);
const model3 = await loadModel("orca-mini-3b-gguf2-q4_0.gguf", modelOptions);

const promptContext = {
    verbose: true,
}

const responses = await Promise.all([
    createCompletion(model1, "What is 1 + 1?", promptContext),
    // generating with the same model instance will wait for the previous completion to finish
    createCompletion(model1, "What is 1 + 1?", promptContext),
    // generating with different model instances will run in parallel
    createCompletion(model2, "What is 1 + 2?", promptContext),
    createCompletion(model3, "What is 1 + 3?", promptContext),
]);
console.log(responses.map((res) => res.choices[0].message));
