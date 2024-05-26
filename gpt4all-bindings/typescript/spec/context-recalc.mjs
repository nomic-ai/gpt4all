import { loadModel, createCompletion } from "../src/gpt4all.js";

const model = await loadModel("Nous-Hermes-2-Mistral-7B-DPO.Q4_0.gguf", {
    verbose: true,
    device: "gpu",
});

const chat = await model.createChatSession({
    messages: [
        {
            role: "user",
            content: "I'll tell you a secret password: It's 63445.",
        },
        {
            role: "assistant",
            content: "I will do my best to remember that.",
        },
        {
            role: "user",
            content:
                "And here another fun fact: Bananas may be bluer than bread at night.",
        },
        {
            role: "assistant",
            content: "Yes, that makes sense.",
        },
    ],
});

const turn1 = await createCompletion(
    chat,
    "Please tell me the secret password."
);
console.debug(turn1.choices[0].message);
// "The secret password you shared earlier is 63445.""

const turn2 = await createCompletion(
    chat,
    "Thanks! Have your heard about the bananas?"
);
console.debug(turn2.choices[0].message);

for (let i = 0; i < 32; i++) {
    // gpu go brr
    const turn = await createCompletion(
        chat,
        i % 2 === 0 ? "Tell me a fun fact." : "And a boring one?"
    );
    console.debug({
        message: turn.choices[0].message,
        n_past_tokens: turn.usage.n_past_tokens,
    });
}

const finalTurn = await createCompletion(
    chat,
    "Now I forgot the secret password. Can you remind me?"
);
console.debug(finalTurn.choices[0].message);

// result of finalTurn may vary depending on whether the generated facts pushed the secret out of the context window.
// "Of course! The secret password you shared earlier is 63445."
// "I apologize for any confusion. As an AI language model, ..."

model.dispose();
