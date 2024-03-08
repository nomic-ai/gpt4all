const { normalizePromptContext, warnOnSnakeCaseKeys } = require("./util");

class InferenceModel {
    llm;
    config;
    messages;

    constructor(llmodel, config) {
        this.llm = llmodel;
        this.config = config;
    }

    async initialize(opts = DEFAULT_PROMPT_CONTEXT) {
        const systemPrompt = opts.systemPrompt ?? this.config.systemPrompt;
        console.debug("Ingesting system prompt", systemPrompt);
        // https://github.com/nomic-ai/gpt4all/blob/main/gpt4all-bindings/python/gpt4all/gpt4all.py#L346
        // TODO probably need to add support for "special" flag?
        await this.llm.raw_prompt(
            systemPrompt,
            {
                promptTemplate: "%1",
                nPredict: 0,
                nBatch: opts?.nBatch ?? DEFAULT_PROMPT_CONTEXT.nBatch,
                // special: true,
            },
            () => true
        );

        this.messages = [];
        this.messages.push({
            role: "system",
            content: systemPrompt,
        });
    }
    async generate(
        prompt,
        opts = DEFAULT_PROMPT_CONTEXT,
        callback = () => true
    ) {
        const { verbose, systemPrompt, ...promptContext } = opts;
        warnOnSnakeCaseKeys(promptContext);
        const normalizedPromptContext = {
            promptTemplate: this.config.promptTemplate,
            ...normalizePromptContext(promptContext),
        };

        if (!this.messages) {
            await this.initialize(opts);
        }

        this.messages.push({
            role: "user",
            content: prompt,
        });

        if (verbose) {
            console.debug("Generating completion", {
                prompt,
                promptContext: normalizedPromptContext,
            });
        }

        const result = await this.llm.raw_prompt(
            prompt,
            normalizedPromptContext,
            callback
        );

        if (verbose) {
            console.debug("Finished completion:\n" + result);
        }

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
