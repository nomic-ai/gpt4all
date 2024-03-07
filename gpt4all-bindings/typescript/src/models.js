const { normalizePromptContext, warnOnSnakeCaseKeys } = require("./util");

class InferenceModel {
    llm;
    config;
    messages;

    constructor(llmodel, config) {
        this.llm = llmodel;
        this.config = config;
    }

    async initialize(promptContext) {
        // https://github.com/nomic-ai/gpt4all/blob/main/gpt4all-bindings/python/gpt4all/gpt4all.py#L346
        // TODO probably need to add support for "special" flag?
        // console.debug("Ingesting system prompt", this.config);
        await this.llm.raw_prompt(
            this.config.systemPrompt,
            {
                promptTemplate: "%1",
                nPredict: 0,
                nBatch: promptContext.nBatch,
                // special: true,
            },
            () => true
        );

        this.messages = [];
        this.messages.push({
            role: "system",
            content: this.config.systemPrompt,
        });
    }
    async generate(prompt, promptContext, callback) {
        warnOnSnakeCaseKeys(promptContext);
        const normalizedPromptContext = {
            promptTemplate: this.config.promptTemplate,
            ...normalizePromptContext(promptContext),
        };

        if (!this.messages) {
            await this.initialize(normalizedPromptContext);
        }

        this.messages.push({
            role: "user",
            content: prompt,
        });

        const result = await this.llm.raw_prompt(
            prompt,
            normalizedPromptContext,
            callback
        );

        this.messages.push({
            role: "assistant",
            content: result,
        });

        return result;
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
