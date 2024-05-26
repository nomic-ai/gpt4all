import {
    loadModel,
    createCompletion,
    createCompletionStream,
    createCompletionGenerator,
} from "../src/gpt4all.js";

const model = await loadModel("Phi-3-mini-4k-instruct.Q4_0.gguf", {
    device: "cpu",
});

const prompt = "Tell a short story but only use emojis. Three sentences max.";

const result = await createCompletion(model, prompt, {
    onResponseToken: (tokens) => {
        console.debug(tokens)
    },
});

console.debug(result.choices[0].message);

process.stdout.write("### Stream:");
const stream = createCompletionStream(model, prompt);
stream.tokens.on("data", (data) => {
    process.stdout.write(data);
});
await stream.result;
process.stdout.write("\n");

process.stdout.write("### Generator:");
const gen = createCompletionGenerator(model, prompt);
for await (const chunk of gen) {
    process.stdout.write(chunk);
}


model.dispose();
