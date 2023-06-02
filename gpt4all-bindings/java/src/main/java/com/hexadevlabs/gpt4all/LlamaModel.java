package com.hexadevlabs.gpt4all;

import jnr.ffi.Pointer;

import java.nio.file.Path;

public class LlamaModel extends LLModel {


    /**
     * For example ggml-vicuna-7b-1.1-q4_2.bin
     * @param modelPath The path for the model weight file
     */
    public LlamaModel(Path modelPath) {
        super( modelPath, LLModelType.LLAMA);
    }

    protected Pointer createModel() {
        return library.llmodel_llama_create();
    }

    @Override
    public void close() throws Exception {
        library.llmodel_llama_destroy(model);
    }
}
