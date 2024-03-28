import { loadModel, createCompletion } from "../src/gpt4all.js";

const model1 = await loadModel("Nous-Hermes-2-Mistral-7B-DPO.Q4_0.gguf", {
    device: "gpu",
    nCtx: 4096,
});

const chat1 = await model1.createChatSession({
    temperature: 0.8,
    topP: 0.7,
    topK: 60,
});

const chat1turn1 = await createCompletion(
    chat1,
    "Outline a short story concept for adults. About why bananas are rather blue than bread is green at night sometimes. Not too long."
);
console.debug(chat1turn1.choices[0].message);

const chat1turn2 = await createCompletion(
    chat1,
    "Lets sprinkle some plot twists. And a cliffhanger at the end."
);
console.debug(chat1turn2.choices[0].message);

const chat1turn3 = await createCompletion(
    chat1,
    "Analyze your plot. Find the weak points."
);
console.debug(chat1turn3.choices[0].message);

const chat1turn4 = await createCompletion(
    chat1,
    "Rewrite it based on the analysis."
);
console.debug(chat1turn4.choices[0].message);

model1.dispose();

const model2 = await loadModel("gpt4all-falcon-newbpe-q4_0.gguf", {
    device: "gpu",
});

const chat2 = await model2.createChatSession({
    messages: chat1.messages,
});

const chat2turn1 = await createCompletion(
    chat2,
    "Give three ideas how this plot could be improved."
);
console.debug(chat2turn1.choices[0].message);

const chat2turn2 = await createCompletion(
    chat2,
    "Revise the plot, applying your ideas."
);
console.debug(chat2turn2.choices[0].message);

model2.dispose();
