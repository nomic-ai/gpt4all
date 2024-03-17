const { DEFAULT_PROMPT_CONTEXT } = require("./config");
const { ChatSession } = require("./chat-session");

class InferenceModel {
    llm;
    modelName;
    config;
    activeChatSession;
    nPast = 0;

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

    async generate(prompt, options = DEFAULT_PROMPT_CONTEXT, onToken) {
        const { verbose, ...otherOptions } = options;
        const promptContext = {
            promptTemplate: this.config.promptTemplate,
            nPast: otherOptions.nPast ?? this.nPast,
            temp:
                otherOptions.temp ??
                otherOptions.temperature ??
                DEFAULT_PROMPT_CONTEXT.temp,
            ...otherOptions,
        };
        
        // allow user to set nPast manually, to keep closer control
        if (promptContext.nPast !== this.nPast) {
            this.nPast = promptContext.nPast;
        }

        if (verbose) {
            console.debug("Generating completion", {
                prompt,
                promptContext,
            });
        }

        let tokensGenerated = 0;

        const response = await this.llm.infer(
            prompt,
            promptContext,
            (tokenId, text, fullText) => {
                let continueGeneration = true;

                if (onToken) {
                    // don't wanna cancel the generation unless user explicitly returns false
                    continueGeneration =
                        onToken(tokenId, text, fullText) !== false;
                }

                tokensGenerated++;
                return continueGeneration;
            }
        );

        this.nPast = response.nPast;

        const result = {
            model: this.modelName,
            usage: {
                // TODO find a way to determine input token count reliably.
                // using nPastDelta fails when the context window is exceeded.
                // prompt_tokens: nPastDelta - tokensGenerated,
                // total_tokens: nPastDelta,
                prompt_tokens: 0,
                total_tokens: 0,
                completion_tokens: tokensGenerated,
                n_past_tokens: this.nPast,
            },
            message: response.text,
        };

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
    MIN_DIMENSIONALITY = 64
    constructor(llmodel, config) {
        this.llm = llmodel;
        this.config = config;
    }

    embed(text, prefix, dimensionality, do_mean, atlas) {
        return this.llm.embed(
            text, 
            prefix,
            dimensionality,
            do_mean,
            atlas
        );
    }

    dispose() {
        this.llm.dispose();
    }
}

module.exports = {
    InferenceModel,
    EmbeddingModel,
};
