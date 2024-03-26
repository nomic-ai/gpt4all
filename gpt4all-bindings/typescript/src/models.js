const { DEFAULT_PROMPT_CONTEXT } = require("./config");
const { ChatSession } = require("./chat-session");
const { prepareMessagesForIngest } = require("./util");

class InferenceModel {
    llm;
    modelName;
    config;
    activeChatSession;

    constructor(llmodel, config) {
        this.llm = llmodel;
        this.config = config;
        this.modelName = this.llm.name();
    }

    async createChatSession(options) {
        const chatSession = new ChatSession(this, options);
        await chatSession.initialize();
        this.activeChatSession = chatSession;
        return this.activeChatSession;
    }

    async generate(input, options = DEFAULT_PROMPT_CONTEXT) {
        const { verbose, ...otherOptions } = options;
        const promptContext = {
            promptTemplate: this.config.promptTemplate,
            temp:
                otherOptions.temp ??
                otherOptions.temperature ??
                DEFAULT_PROMPT_CONTEXT.temp,
            ...otherOptions,
        };
        
        if (promptContext.nPast < 0) {
            throw new Error("nPast must be a non-negative integer.");
        }

        if (verbose) {
            console.debug("Generating completion", {
                input,
                promptContext,
            });
        }

        let prompt = input;
        let nPast = promptContext.nPast;
        let tokensIngested = 0;

        if (Array.isArray(input)) {
            // assuming input is a messages array
            // -> tailing user message will be used as the final prompt. its required.
            // -> leading system message will be ingested as systemPrompt, further system messages will be ignored
            // -> all other messages will be ingested with fakeReply
            // -> model/context will only be kept for this completion; "stateless"
            nPast = 0;
            const messages = [...input];
            const lastMessage = input[input.length - 1];
            if (lastMessage.role !== "user") {
                // this is most likely a user error
                throw new Error("The final message must be of role 'user'.");
            }
            if (input[0].role === "system") {
                // needs to be a pre-templated prompt ala '<|im_start|>system\nYou are an advanced mathematician.\n<|im_end|>\n'
                const systemPrompt = input[0].content;
                const systemRes = await this.llm.infer(systemPrompt, {
                    promptTemplate: "%1",
                    nPredict: 0,
                    special: true,
                });
                nPast = systemRes.nPast;
                tokensIngested += systemRes.tokensIngested;
                messages.shift();
            }

            prompt = lastMessage.content;
            const messagesToIngest = messages.slice(0, input.length - 1);
            const turns = prepareMessagesForIngest(messagesToIngest);

            for (const turn of turns) {
                const turnRes = await this.llm.infer(turn.user, {
                    ...promptContext,
                    nPast,
                    fakeReply: turn.assistant,
                });
                tokensIngested += turnRes.tokensIngested;
                nPast = turnRes.nPast;
            }
        }

        let tokensGenerated = 0;

        const result = await this.llm.infer(prompt, {
            ...promptContext,
            nPast,
            onPromptToken: (tokenId) => {
                let continueIngestion = true;
                tokensIngested++;
                if (options.onPromptToken) {
                    // catch errors because if they go through cpp they will loose stacktraces
                    try {
                        // don't cancel ingestion unless user explicitly returns false
                        continueIngestion =
                            options.onPromptToken(tokenId) !== false;
                    } catch (e) {
                        console.error("Error in onPromptToken callback", e);
                        continueIngestion = false;
                    }
                }
                return continueIngestion;
            },
            onResponseToken: (tokenId, token) => {
                let continueGeneration = true;
                tokensGenerated++;
                if (options.onResponseToken) {
                    try {
                        // don't cancel the generation unless user explicitly returns false
                        continueGeneration =
                            options.onResponseToken(tokenId, token) !== false;
                    } catch (err) {
                        console.error("Error in onResponseToken callback", err);
                        continueGeneration = false;
                    }
                }
                return continueGeneration;
            },
        });

        result.tokensGenerated = tokensGenerated;
        result.tokensIngested = tokensIngested;

        if (verbose) {
            console.debug("Finished completion:\n", result);
        }

        return result;
    }

    dispose() {
        this.llm.dispose();
    }
}

class EmbeddingModel {
    llm;
    config;
    MIN_DIMENSIONALITY = 64;
    constructor(llmodel, config) {
        this.llm = llmodel;
        this.config = config;
    }

    embed(text, prefix, dimensionality, do_mean, atlas) {
        return this.llm.embed(text, prefix, dimensionality, do_mean, atlas);
    }

    dispose() {
        this.llm.dispose();
    }
}

module.exports = {
    InferenceModel,
    EmbeddingModel,
};
