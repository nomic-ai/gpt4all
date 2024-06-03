import { loadModel, createCompletion } from "../src/gpt4all.js";

const model = await loadModel("orca-mini-3b-gguf2-q4_0.gguf", {
    verbose: true,
    device: "gpu",
});

const chat = await model.createChatSession();

await createCompletion(
    chat,
    "Why are bananas rather blue than bread at night sometimes?",
    {
        verbose: true,
    }
);
await createCompletion(chat, "Are you sure?", {
    verbose: true,
});
