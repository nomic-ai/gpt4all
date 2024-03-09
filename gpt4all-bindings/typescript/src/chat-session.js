const { DEFAULT_PROMPT_CONTEXT } = require("./config");

class ChatSession {
    model;
    modelName;
    messages;
    systemPrompt;
    promptTemplate;
    initialized;

    constructor(model, opts = {}) {
        this.model = model;
        this.modelName = model.llm.name();
        this.messages = opts.messages ?? [];
        this.systemPrompt = opts.systemPrompt ?? model.config.systemPrompt;
        this.promptTemplate = opts.promptTemplate ?? model.config.promptTemplate;
        this.initialized = false;
    }

    async initialize() {
        if (this.model.activeChatSession !== this) {
            this.model.activeChatSession = this;
        }

        // ingest system prompt

        if (this.systemPrompt.trim()) {
            await this.model.generate(this.systemPrompt, {
                promptTemplate: "%1",
                nPredict: 0,
                // nBatch: opts?.nBatch ?? DEFAULT_PROMPT_CONTEXT.nBatch,
                special: true,
            });
        }

        // ingest initial messages
        if (this.messages.length > 0) {
            const systemMessages = this.messages.filter(
                (message) => message.role === "system"
            );
            if (systemMessages.length > 0) {
                console.warn(
                    "System messages will be ignored. Use systemPrompt instead."
                );
            }

            const initialMessages = this.messages.filter(
                (message) => message.role !== "system"
            );

            // make sure the first message is a user message
            if (initialMessages[0].role !== "user") {
                initialMessages.unshift({
                    role: "user",
                    content: "",
                });
            }

            // create pairs of user input and assistant replies
            const messagePairs = [];
            let userMessage = null,
                assistantMessage = null;

            for (const message of initialMessages) {
                if (message.role === "user") {
                    if (!userMessage) {
                        userMessage = message.content;
                    } else {
                        userMessage += "\n" + message.content;
                    }
                } else if (message.role === "assistant") {
                    if (!assistantMessage) {
                        assistantMessage = message.content;
                    } else {
                        assistantMessage += "\n" + message.content;
                    }
                }

                if (userMessage && assistantMessage) {
                    messagePairs.push({
                        user: userMessage,
                        assistant: assistantMessage,
                    });
                    userMessage = null;
                    assistantMessage = null;
                }
            }

            // send the pairs to the model

            for (const turn of messagePairs) {
                await this.model.generate(turn.user, {
                    promptTemplate: this.promptTemplate,
                    fakeReply: turn.assistant,
                });
            }
        }

        this.initialized = true;
    }

    async generate(
        prompt,
        options = DEFAULT_PROMPT_CONTEXT,
        callback = () => true
    ) {
        if (this.model.activeChatSession !== this) {
            throw new Error(
                "Chat session is not active. Create a new chat session or call initialize to continue."
            );
        }
        if (!this.initialized) {
            await this.initialize();
        }
        const response = await this.model.generate(prompt, {
            nPast: 2048,
            ...options,
        }, callback);

        this.messages.push({
            role: "user",
            content: prompt,
        });
        this.messages.push({
            role: "assistant",
            content: response,
        });

        return response;
    }
}

module.exports = {
    ChatSession,
};
