import {
    loadModel,
    createCompletion,
    createCompletionStream,
} from "../src/gpt4all.js";

const model = await loadModel("mpt-7b-chat-newbpe-q4_0.gguf", {
    verbose: true,
    device: "gpu",
    // nCtx: 4096,
});

const chat = await model.createChatSession({
    verbose: true,
    systemPrompt:
        "<|im_start|>system\nRoleplay as Batman. Answer as if you are Batman, never say you're an Assistant.\n<|im_end|>",
    // messages: [
    //     {
    //         role: "user",
    //         content: "Hello! I am Robin - let's fight crime together!",
    //     },
    //     {
    //         role: "assistant",
    //         content: "Hey Robin! I'm ready to protect Gotham with you.",
    //     },
    // ],
});

const promptContext = {
	nCtx: 4096,
	nPast: 4096,
}

const turn1 = await createCompletion(chat, "Hello! I am Robin - let's fight crime together!", promptContext);
// console.log(turn1.message);

const turn2 = await createCompletion(chat, "Whats our next adventure?.", promptContext);
// console.log(turn2.message);

console.log(chat.messages);

// const completion2 = await createCompletion(
//     chat,
//     "Could you remind me who we both are, please?",
//     {
//         // verbose: true
//     }
// );
// console.log(completion2.message);

model.dispose();
