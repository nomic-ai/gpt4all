import gpt from "../src/gpt4all.js";

const model = await gpt.loadModel("mistral-7b-openorca.gguf2.Q4_0.gguf", {
    device: "gpu",
});

process.stdout.write("### Stream:");
const stream = gpt.createCompletionStream(model, "How are you?");
stream.tokens.on("data", (data) => {
    process.stdout.write(data);
});
await stream.result;
process.stdout.write("\n");

process.stdout.write("### Stream with pipe:");
const stream2 = gpt.createCompletionStream(
    model,
    "Please say something nice about node streams."
);
stream2.tokens.pipe(process.stdout);
await stream2.result;
process.stdout.write("\n");

process.stdout.write("### Generator:");
const gen = gpt.createCompletionGenerator(model, "generators instead?");
for await (const chunk of gen) {
    process.stdout.write(chunk);
}

process.stdout.write("\n");

process.stdout.write("### Callback:");
await gpt.createCompletion(model, "Why not just callbacks?", {
    onResponseToken: (tokenId, token) => {
        process.stdout.write(token);
    },
});
process.stdout.write("\n");

process.stdout.write("### 2nd Generator:");
const gen2 = gpt.createCompletionGenerator(
    model,
    "If 3 + 3 is 5, what is 2 + 2?"
);
for await (const chunk of gen2) {
    process.stdout.write(chunk);
}
process.stdout.write("\n");
// console.log("done");
model.dispose();
