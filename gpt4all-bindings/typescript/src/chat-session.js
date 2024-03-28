const { DEFAULT_PROMPT_CONTEXT } = require("./config");
const { prepareMessagesForIngest } = require("./util");

class ChatSession {
    model;
    modelName;
    /**
     * @type {import('./gpt4all').ChatMessage[]}
     */
    messages;
    /**
     * @type {string}
     */
    systemPrompt;
    /**
     * @type {import('./gpt4all').LLModelPromptContext}
     */
    promptContext;
    /**
     * @type {boolean}
     */
    initialized;

    constructor(model, chatSessionOpts = {}) {
        const { messages, systemPrompt, ...sessionDefaultPromptContext } =
            chatSessionOpts;
        this.model = model;
        this.modelName = model.llm.name();
        this.messages = messages ?? [];
        this.systemPrompt = systemPrompt ?? model.config.systemPrompt;
        this.initialized = false;
        this.promptContext = {
            ...DEFAULT_PROMPT_CONTEXT,
            ...sessionDefaultPromptContext,
            nPast: 0,
        };
    }

    async initialize(completionOpts = {}) {
        if (this.model.activeChatSession !== this) {
            this.model.activeChatSession = this;
        }

        let tokensIngested = 0;

        // ingest system prompt

        if (this.systemPrompt) {
            const systemRes = await this.model.generate(this.systemPrompt, {
                promptTemplate: "%1",
                nPredict: 0,
                special: true,
                nBatch: this.promptContext.nBatch,
                // verbose: true,
            });
            tokensIngested += systemRes.tokensIngested;
            this.promptContext.nPast = systemRes.nPast;
        }

        // ingest initial messages
        if (this.messages.length > 0) {
            tokensIngested += await this.ingestMessages(
                this.messages,
                completionOpts
            );
        }

        this.initialized = true;

        return tokensIngested;
    }

    async ingestMessages(messages, completionOpts = {}) {
        const turns = prepareMessagesForIngest(messages);

        // send the message pairs to the model
        let tokensIngested = 0;

        for (const turn of turns) {
            const turnRes = await this.model.generate(turn.user, {
                ...this.promptContext,
                ...completionOpts,
                fakeReply: turn.assistant,
            });
            tokensIngested += turnRes.tokensIngested;
            this.promptContext.nPast = turnRes.nPast;
        }
        return tokensIngested;
    }

    async generate(input, completionOpts = {}) {
        if (this.model.activeChatSession !== this) {
            throw new Error(
                "Chat session is not active. Create a new chat session or call initialize to continue."
            );
        }
        if (completionOpts.nPast > this.promptContext.nPast) {
            throw new Error(
                `nPast cannot be greater than ${this.promptContext.nPast}.`
            );
        }
        let tokensIngested = 0;

        if (!this.initialized) {
            tokensIngested += await this.initialize(completionOpts);
        }

        let prompt = input;

        if (Array.isArray(input)) {
            // assuming input is a messages array
            // -> tailing user message will be used as the final prompt. its optional.
            // -> all system messages will be ignored.
            // -> all other messages will be ingested with fakeReply
            // -> user/assistant messages will be pushed into the messages array

            let tailingUserMessage = "";
            let messagesToIngest = input;

            const lastMessage = input[input.length - 1];
            if (lastMessage.role === "user") {
                tailingUserMessage = lastMessage.content;
                messagesToIngest = input.slice(0, input.length - 1);
            }

            if (messagesToIngest.length > 0) {
                tokensIngested += await this.ingestMessages(
                    messagesToIngest,
                    completionOpts
                );
                this.messages.push(...messagesToIngest);
            }

            if (tailingUserMessage) {
                prompt = tailingUserMessage;
            } else {
                return {
                    text: "",
                    nPast: this.promptContext.nPast,
                    tokensIngested,
                    tokensGenerated: 0,
                };
            }
        }

        const result = await this.model.generate(prompt, {
            ...this.promptContext,
            ...completionOpts,
        });

        this.promptContext.nPast = result.nPast;
        result.tokensIngested += tokensIngested;

        this.messages.push({
            role: "user",
            content: prompt,
        });
        this.messages.push({
            role: "assistant",
            content: result.text,
        });

        return result;
    }
}

module.exports = {
    ChatSession,
};
