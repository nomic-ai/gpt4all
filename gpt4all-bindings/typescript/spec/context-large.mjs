import { promises as fs } from "node:fs";
import { loadModel, createCompletion } from "../src/gpt4all.js";

const model = await loadModel("Nous-Hermes-2-Mistral-7B-DPO.Q4_0.gguf", {
    verbose: true,
    device: "gpu",
    nCtx: 32768,
});

const typeDefSource = await fs.readFile("./src/gpt4all.d.ts", "utf-8");

const res = await createCompletion(
    model,
    "Here are the type definitions for the GPT4All API:\n\n" +
        typeDefSource +
        "\n\nHow do I create a completion with a really large context window?",
    {
        verbose: true,
    }
);
console.debug(res.choices[0].message);
