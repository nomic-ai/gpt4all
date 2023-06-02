package com.hexadevlabs.gpt4all;

import jnr.ffi.Pointer;

import java.nio.file.Path;

public class MPTModel extends LLModel {
    /**
     * For example ggml-mpt-7b-instruct.bin
     * @param modelPath  The path for the model weight file
     */
    public MPTModel(Path modelPath) {
        super(modelPath, LLModelType.MPT);
    }
    protected Pointer createModel() {
        return library.llmodel_mpt_create();
    }

    @Override
    public void close() throws Exception {
        library.llmodel_mpt_destroy(model);
    }
}
