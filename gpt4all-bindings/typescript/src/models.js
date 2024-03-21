const { normalizePromptContext, warnOnSnakeCaseKeys } = require('./util');

class InferenceModel {
    llm;
    config;

    constructor(llmodel, config) {
        this.llm = llmodel;
        this.config = config;
    }

    async generate(prompt, promptContext,callback) {
        warnOnSnakeCaseKeys(promptContext);
        const normalizedPromptContext = normalizePromptContext(promptContext);
        const result = this.llm.raw_prompt(prompt, normalizedPromptContext,callback);
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
        return this.llm.embed(text)
    }

    dispose() {
        this.llm.dispose();
    }
}


module.exports = {
    InferenceModel,
    EmbeddingModel,
};
