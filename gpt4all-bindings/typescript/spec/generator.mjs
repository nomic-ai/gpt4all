import gpt from "../src/gpt4all.js";

const model = await gpt.loadModel("mistral-7b-openorca.gguf2.Q4_0.gguf", {
    device: "gpu",
});

process.stdout.write("Stream:");

const stream = gpt.createCompletionStream(model, "How are you ?", {
    nPredict: 2048,
});
stream.tokens.on("data", (data) => {
    process.stdout.write(data);
});
await stream.result;
process.stdout.write("\n");

process.stdout.write("Generator:");
const gen = gpt.createCompletionGenerator(model, "You sure ?", {
    nPredict: 2048,
});
for await (const chunk of gen) {
    process.stdout.write(chunk);
}

process.stdout.write("\n");

process.stdout.write("Callback:");
await gpt.createCompletion(model, "You sure you sure?", {
    onToken: (tokenId, tokenText) => {
        process.stdout.write(tokenText);
    },
});
process.stdout.write("\n");

process.stdout.write("2nd Generator:");
const gen2 = gpt.createCompletionGenerator(
    model,
    "If 3 + 3 is 5, what is 2 + 2?",
    { nPredict: 2048 }
);
for await (const chunk of gen2) {
    process.stdout.write(chunk);
}
process.stdout.write("\n");
// console.log("done");
model.dispose();
