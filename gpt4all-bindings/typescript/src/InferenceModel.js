class InferenceModel {
    llmodel;
    config;

    constructor(llmodel, config) {
        this.llmodel = llmodel;
        this.config = config;
    }

    generate(prompt, options) {
        const result = this.llmodel.raw_prompt(prompt, options);
        return result;

        // return promisifiedRawPrompt.then((response) => {
        //     return {
        //         llmodel: llmodel.name(),
        //         usage: {
        //             prompt_tokens: fullPrompt.length,
        //             completion_tokens: response.length, //TODO
        //             total_tokens: fullPrompt.length + response.length, //TODO
        //         },
        //         choices: [
        //             {
        //                 message: {
        //                     role: "assistant",
        //                     content: response,
        //                 },
        //             },
        //         ],
        //     };
        // });
    }
}

module.exports = {
    InferenceModel,
};
