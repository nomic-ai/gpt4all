import {
    loadModel,
    createCompletion,
    createCompletionStream,
    createCompletionGenerator,
} from "../src/gpt4all.js";

const model = await loadModel("mistral-7b-openorca.gguf2.Q4_0.gguf", {
    device: "gpu",
});

process.stdout.write("### Stream:");
const stream = createCompletionStream(model, "How are you?");
stream.tokens.on("data", (data) => {
    process.stdout.write(data);
});
await stream.result;
process.stdout.write("\n");

process.stdout.write("### Stream with pipe:");
const stream2 = createCompletionStream(
    model,
    "Please say something nice about node streams."
);
stream2.tokens.pipe(process.stdout);
const stream2Res = await stream2.result;
process.stdout.write("\n");

process.stdout.write("### Generator:");
const gen = createCompletionGenerator(model, "generators instead?", {
    nPast: stream2Res.usage.n_past_tokens,
});
for await (const chunk of gen) {
    process.stdout.write(chunk);
}

process.stdout.write("\n");

process.stdout.write("### Callback:");
await createCompletion(model, "Why not just callbacks?", {
    onResponseToken: (tokenId, token) => {
        process.stdout.write(token);
    },
});
process.stdout.write("\n");

process.stdout.write("### 2nd Generator:");
const gen2 = createCompletionGenerator(model, "If 3 + 3 is 5, what is 2 + 2?");

let chunk = await gen2.next();
while (!chunk.done) {
    process.stdout.write(chunk.value);
    chunk = await gen2.next();
}
process.stdout.write("\n");
console.debug("generator finished", chunk);
model.dispose();
