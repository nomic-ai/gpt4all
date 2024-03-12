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
console.log(turn1.message);

model.dispose();
