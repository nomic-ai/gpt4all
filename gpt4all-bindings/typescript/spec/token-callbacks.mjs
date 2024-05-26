import { loadModel, createCompletion } from "../src/gpt4all.js";

const model = await loadModel("Phi-3-mini-4k-instruct.Q4_0.gguf", {
    verbose: true,
    device: "gpu",
});

const res = await createCompletion(
    model,
    "I've got three 🍣 - What shall I name them?",
    {
        onPromptToken: (tokenId) => {
            console.debug("onPromptToken", { tokenId });
            // errors within the callback will cancel ingestion, inference will still run
            throw new Error("This is an error");
            // const foo = thisMethodDoesNotExist();
            // returning false will cancel as well
            // return false;
        },
        onResponseTokens: ({ tokenIds, text }) => {
            // console.debug("onResponseToken", { tokenIds, text });
            process.stdout.write(text);
            // same applies here
        },
    }
);

console.debug("Output:", {
    usage: res.usage,
    message: res.choices[0].message,
});
