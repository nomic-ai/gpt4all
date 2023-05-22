#include "llmodel_c.h"

#include "gptj.h"
#include "llamamodel.h"
#include "mpt.h"

struct LLModelWrapper {
    LLModel *llModel = nullptr;
    LLModel::PromptContext promptContext;
};

llmodel_model llmodel_gptj_create()
{
    LLModelWrapper *wrapper = new LLModelWrapper;
    wrapper->llModel = new GPTJ;
    return reinterpret_cast<void*>(wrapper);
}

void llmodel_gptj_destroy(llmodel_model gptj)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(gptj);
    delete wrapper->llModel;
    delete wrapper;
}

llmodel_model llmodel_mpt_create()
{
    LLModelWrapper *wrapper = new LLModelWrapper;
    wrapper->llModel = new MPT;
    return reinterpret_cast<void*>(wrapper);
}

void llmodel_mpt_destroy(llmodel_model mpt)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(mpt);
    delete wrapper->llModel;
    delete wrapper;
}

llmodel_model llmodel_llama_create()
{
    LLModelWrapper *wrapper = new LLModelWrapper;
    wrapper->llModel = new LLamaModel;
    return reinterpret_cast<void*>(wrapper);
}

void llmodel_llama_destroy(llmodel_model llama)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(llama);
    delete wrapper->llModel;
    delete wrapper;
}

llmodel_model llmodel_model_create(const char *model_path) {

    uint32_t magic;
    llmodel_model model;
    FILE *f = fopen(model_path, "rb");
    fread(&magic, sizeof(magic), 1, f);

    if (magic == 0x67676d6c) { model = llmodel_gptj_create();  }
    else if (magic == 0x67676a74) { model = llmodel_llama_create(); }
    else if (magic == 0x67676d6d) { model = llmodel_mpt_create();   }
    else  {fprintf(stderr, "Invalid model file\n");}
    fclose(f);
    return model;
}

void llmodel_model_destroy(llmodel_model model) {

    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    const std::type_info &modelTypeInfo = typeid(*wrapper->llModel);

    if (modelTypeInfo == typeid(GPTJ))       { llmodel_gptj_destroy(model);  }
    if (modelTypeInfo == typeid(LLamaModel)) { llmodel_llama_destroy(model); }
    if (modelTypeInfo == typeid(MPT))        { llmodel_mpt_destroy(model);   }
}

bool llmodel_loadModel(llmodel_model model, const char *model_path)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    return wrapper->llModel->loadModel(model_path);
}

bool llmodel_isModelLoaded(llmodel_model model)
{
    const auto *llm = reinterpret_cast<LLModelWrapper*>(model)->llModel;
    return llm->isModelLoaded();
}

uint64_t llmodel_get_state_size(llmodel_model model)
{
    const auto *llm = reinterpret_cast<LLModelWrapper*>(model)->llModel;
    return llm->stateSize();
}

uint64_t llmodel_save_state_data(llmodel_model model, uint8_t *dest)
{
    const auto *llm = reinterpret_cast<LLModelWrapper*>(model)->llModel;
    return llm->saveState(dest);
}

uint64_t llmodel_restore_state_data(llmodel_model model, const uint8_t *src)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    return wrapper->llModel->restoreState(src);
}

// Wrapper functions for the C callbacks
bool prompt_wrapper(int32_t token_id, void *user_data) {
    llmodel_prompt_callback callback = reinterpret_cast<llmodel_prompt_callback>(user_data);
    return callback(token_id);
}

bool response_wrapper(int32_t token_id, const std::string &response, void *user_data) {
    llmodel_response_callback callback = reinterpret_cast<llmodel_response_callback>(user_data);
    return callback(token_id, response.c_str());
}

bool recalculate_wrapper(bool is_recalculating, void *user_data) {
    llmodel_recalculate_callback callback = reinterpret_cast<llmodel_recalculate_callback>(user_data);
    return callback(is_recalculating);
}

void llmodel_prompt(llmodel_model model, const char *prompt,
                    llmodel_prompt_callback prompt_callback,
                    llmodel_response_callback response_callback,
                    llmodel_recalculate_callback recalculate_callback,
                    llmodel_prompt_context *ctx)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);

    // Create std::function wrappers that call the C function pointers
    std::function<bool(int32_t)> prompt_func =
        std::bind(&prompt_wrapper, std::placeholders::_1, reinterpret_cast<void*>(prompt_callback));
    std::function<bool(int32_t, const std::string&)> response_func =
        std::bind(&response_wrapper, std::placeholders::_1, std::placeholders::_2, reinterpret_cast<void*>(response_callback));
    std::function<bool(bool)> recalc_func =
        std::bind(&recalculate_wrapper, std::placeholders::_1, reinterpret_cast<void*>(recalculate_callback));

    // Copy the C prompt context
    wrapper->promptContext.n_past = ctx->n_past;
    wrapper->promptContext.n_ctx = ctx->n_ctx;
    wrapper->promptContext.n_predict = ctx->n_predict;
    wrapper->promptContext.top_k = ctx->top_k;
    wrapper->promptContext.top_p = ctx->top_p;
    wrapper->promptContext.temp = ctx->temp;
    wrapper->promptContext.n_batch = ctx->n_batch;
    wrapper->promptContext.repeat_penalty = ctx->repeat_penalty;
    wrapper->promptContext.repeat_last_n = ctx->repeat_last_n;
    wrapper->promptContext.contextErase = ctx->context_erase;

    // Call the C++ prompt method
    wrapper->llModel->prompt(prompt, prompt_func, response_func, recalc_func, wrapper->promptContext);

    // Update the C context by giving access to the wrappers raw pointers to std::vector data
    // which involves no copies
    ctx->logits = wrapper->promptContext.logits.data();
    ctx->logits_size = wrapper->promptContext.logits.size();
    ctx->tokens = wrapper->promptContext.tokens.data();
    ctx->tokens_size = wrapper->promptContext.tokens.size();

    // Update the rest of the C prompt context
    ctx->n_past = wrapper->promptContext.n_past;
    ctx->n_ctx = wrapper->promptContext.n_ctx;
    ctx->n_predict = wrapper->promptContext.n_predict;
    ctx->top_k = wrapper->promptContext.top_k;
    ctx->top_p = wrapper->promptContext.top_p;
    ctx->temp = wrapper->promptContext.temp;
    ctx->n_batch = wrapper->promptContext.n_batch;
    ctx->repeat_penalty = wrapper->promptContext.repeat_penalty;
    ctx->repeat_last_n = wrapper->promptContext.repeat_last_n;
    ctx->context_erase = wrapper->promptContext.contextErase;
}

void llmodel_setThreadCount(llmodel_model model, int32_t n_threads)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    wrapper->llModel->setThreadCount(n_threads);
}

int32_t llmodel_threadCount(llmodel_model model)
{
    const auto *llm = reinterpret_cast<LLModelWrapper*>(model)->llModel;
    return llm->threadCount();
}
