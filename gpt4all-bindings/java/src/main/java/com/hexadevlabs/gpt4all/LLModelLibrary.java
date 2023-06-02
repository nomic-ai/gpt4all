package com.hexadevlabs.gpt4all;

import jnr.ffi.Pointer;
import jnr.ffi.Struct;
import jnr.ffi.annotations.Delegate;
import jnr.ffi.annotations.In;
import jnr.ffi.types.u_int64_t;


/**
 * The basic Native library interface the provides all the LLM functions.
 */
public interface LLModelLibrary {

    interface PromptCallback {
        @Delegate
        boolean invoke(int token_id);
    }

    interface ResponseCallback {
        @Delegate
        boolean invoke(int token_id, String response);
    }

    interface RecalculateCallback {
        @Delegate
        boolean invoke(boolean is_recalculating);
    }

    class LLModelPromptContext extends Struct {
        public final Pointer logits = new Pointer();
        public final ssize_t logits_size = new ssize_t();
        public final Pointer tokens = new Pointer();
        public final ssize_t tokens_size = new ssize_t();
        public final int32_t n_past = new int32_t();
        public final int32_t n_ctx = new int32_t();
        public final int32_t n_predict = new int32_t();
        public final int32_t top_k = new int32_t();
        public final Float top_p = new Float();
        public final Float temp = new Float();
        public final int32_t n_batch = new int32_t();
        public final Float repeat_penalty = new Float();
        public final int32_t repeat_last_n = new int32_t();
        public final Float context_erase = new Float();

        public LLModelPromptContext(jnr.ffi.Runtime runtime) {
            super(runtime);
        }
    }

    Pointer llmodel_gptj_create();
    void llmodel_gptj_destroy(Pointer gptj);
    Pointer llmodel_mpt_create();
    void llmodel_mpt_destroy(Pointer mpt);
    Pointer llmodel_llama_create();
    void llmodel_llama_destroy(Pointer llama);
    Pointer llmodel_model_create(String model_path);
    void llmodel_model_destroy(Pointer model);
    boolean llmodel_loadModel(Pointer model, String model_path);
    boolean llmodel_isModelLoaded(Pointer model);
    @u_int64_t long llmodel_get_state_size(Pointer model);
    @u_int64_t long llmodel_save_state_data(Pointer model, Pointer dest);
    @u_int64_t long llmodel_restore_state_data(Pointer model, Pointer src);

    // ctx was an @Out ... without @Out crash
    void llmodel_prompt(Pointer model, String prompt,
                        PromptCallback prompt_callback,
                        ResponseCallback response_callback,
                        RecalculateCallback recalculate_callback,
                        @In LLModelPromptContext ctx);
    void llmodel_setThreadCount(Pointer model, int n_threads);
    int llmodel_threadCount(Pointer model);
}
