const { DEFAULT_PROMPT_CONTEXT } = require("./config");
const { ChatSession } = require("./chat-session");

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

    async generate(
        prompt,
        options = DEFAULT_PROMPT_CONTEXT,
        callback = () => true
    ) {
        const { verbose, ...otherOptions } = options;
        const promptContext = {
            promptTemplate: this.config.promptTemplate,
            ...otherOptions,
        };

        if (verbose) {
            console.debug("Generating completion", {
                prompt,
                promptContext,
            });
        }

        const response = await this.llm.raw_prompt(
            prompt,
            promptContext,
            callback
        );

        if (verbose) {
            console.debug("Finished completion:\n" + response);
        }

        return response;
    }

    dispose() {
        this.llm.dispose();
    }
}

class EmbeddingModel {
    llm;
    config;

    constructor(llmodel, config) {
        this.llm = llmodel;
        this.config = config;
    }

    embed(text) {
        return this.llm.embed(text);
    }

    dispose() {
        this.llm.dispose();
    }
}

module.exports = {
    InferenceModel,
    EmbeddingModel,
};
