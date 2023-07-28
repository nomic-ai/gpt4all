class InferenceModel {
    llm;
    config;

    constructor(llmodel, config) {
        this.llm = llmodel;
        this.config = config;
    }

    async generate(prompt, options) {
        const result = this.llm.raw_prompt(prompt, options, () => {});
        return result;
    }
}

class EmbeddingModel {
    llm;
    config;

    constructor(llmodel, config) {
        this.llm = llmodel;
        this.config = config;
    }

    async embed(text) {
        return this.llm.embed(text)
    }
}


module.exports = {
    InferenceModel,
    EmbeddingModel,
};
