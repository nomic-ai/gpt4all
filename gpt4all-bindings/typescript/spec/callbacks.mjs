import { promises as fs } from "node:fs";
import { loadModel, createCompletion } from "../src/gpt4all.js";

const model = await loadModel("Nous-Hermes-2-Mistral-7B-DPO.Q4_0.gguf", {
    verbose: true,
    device: "gpu",
});

const res = await createCompletion(
    model,
    "I've got three 🍣 - What shall I name them?",
    {
        onPromptToken: (tokenId) => {
            console.debug("onPromptToken", { tokenId });
            // throwing an error will cancel
            throw new Error("This is an error");
            // const foo = thisMethodDoesNotExist();
            // returning false will cancel as well
            // return false;
        },
        onResponseToken: (tokenId, token) => {
            console.debug("onResponseToken", { tokenId, token });
            // same applies here
        },
    }
);

console.debug("Output:", {
    usage: res.usage,
    message: res.choices[0].message,
});
