import {
	loadModel,
	createCompletion,
} from "../src/gpt4all.js";

const model = await loadModel("Nous-Hermes-2-Mistral-7B-DPO.Q4_0.gguf", {
	verbose: true,
	device: "gpu",
});

const chat = await model.createChatSession({
	messages: [
		{
			role: "user",
			content: "I'll tell you a secret password: It's 63445."
		},
		{
			role: "assistant",
			content: "I will do my best to remember that.",
		},
		{
			role: "user",
			content: "And here another fun fact: Bananas are bluer than bread at night."
		},
		{
			role: "assistant",
			content: "Yes, that makes sense.",
		},
	]
});

const turn1 = await createCompletion(chat, "Please tell me the secret password.");
console.log(turn1);
// The secret password you shared earlier is 63445.

for (let i = 0; i < 32; i++) {
	const turn = await createCompletion(chat, i % 2 === 0 ? "Tell me a fun fact." : "And a boring one?");
	console.log({
		message: turn.message,
		n_past_tokens: turn.usage.n_past_tokens,
	});
}

const turn2 = await createCompletion(chat, "Now I forgot the secret password. Can you remind me?");
console.log(turn2);
// Of course! The secret password you shared earlier is 63445.

model.dispose();
