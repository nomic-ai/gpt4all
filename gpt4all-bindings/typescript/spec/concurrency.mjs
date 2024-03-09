// At the moment, from testing this code, concurrent model prompting is not possible.
// Behavior: The last prompt gets answered, but the rest are cancelled
// my experience with threading is not the best, so if anyone who is good is willing to give this a shot,
// maybe this is possible
// INFO: threading with llama.cpp is not the best maybe not even possible, so this will be left here as reference

const model1 = await loadModel("orca-mini-3b-gguf2-q4_0.gguf", {
    verbose: true,
    // device: 'gpu'
});

const model2 = await loadModel("orca-mini-3b-gguf2-q4_0.gguf", {
    verbose: true,
    // device: 'gpu'
});

const responses = await Promise.all([
    createCompletion(model1, "What is 1 + 1?", { verbose: true }),
    // createCompletion(model1, "What is 1 + 1?", { verbose: true }),
    createCompletion(model2, "What is 1 + 1?", { verbose: true }),
]);
console.log(responses.map((res) => res.message));
