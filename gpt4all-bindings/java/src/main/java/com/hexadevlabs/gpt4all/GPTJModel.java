package com.hexadevlabs.gpt4all;

import jnr.ffi.Pointer;

import java.nio.file.Path;

public class GPTJModel extends LLModel {
    /**
     * For example ggml-gpt4all-j-v1.3-groovy.bin
     * @param modelPath The path for the model weight file
     */
    public GPTJModel(Path modelPath) {
        super(modelPath,LLModelType.GPTJ);
    }

    protected Pointer createModel() {
        return library.llmodel_gptj_create();
    }

    @Override
    public void close() throws Exception {
        library.llmodel_gptj_destroy(model);
    }
}