#include "llmodel_c.h"
#include "llmodel.h"

#include <cstring>
#include <cerrno>
#include <utility>


struct LLModelWrapper {
    LLModel *llModel = nullptr;
    LLModel::PromptContext promptContext;
    ~LLModelWrapper() { delete llModel; }
};


thread_local static std::string last_error_message;


llmodel_model llmodel_model_create(const char *model_path) {
    auto fres = llmodel_model_create2(model_path, "auto", nullptr);
    if (!fres) {
        fprintf(stderr, "Invalid model file\n");
    }
    return fres;
}

llmodel_model llmodel_model_create2(const char *model_path, const char *build_variant, llmodel_error *error) {
    auto wrapper = new LLModelWrapper;
    int error_code = 0;

    try {
        wrapper->llModel = LLModel::Implementation::construct(model_path, build_variant);
    } catch (const std::exception& e) {
        error_code = EINVAL;
        last_error_message = e.what();
    }

    if (!wrapper->llModel) {
        delete std::exchange(wrapper, nullptr);
        // Get errno and error message if none
        if (error_code == 0) {
            if (errno != 0) {
                error_code = errno;
                last_error_message = std::strerror(error_code);
            } else {
                error_code = ENOTSUP;
                last_error_message = "Model format not supported (no matching implementation found)";
            }
        }
        // Set error argument
        if (error) {
            error->message = last_error_message.c_str();
            error->code = error_code;
        }
    }
    return reinterpret_cast<llmodel_model*>(wrapper);
}

void llmodel_model_destroy(llmodel_model model) {
    delete reinterpret_cast<LLModelWrapper*>(model);
}

size_t llmodel_required_mem(llmodel_model model, const char *model_path)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    return wrapper->llModel->requiredMem(model_path);
}

bool llmodel_loadModel(llmodel_model model, const char *model_path)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    return wrapper->llModel->loadModel(model_path);
}

bool llmodel_isModelLoaded(llmodel_model model)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    return wrapper->llModel->isModelLoaded();
}

uint64_t llmodel_get_state_size(llmodel_model model)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    return wrapper->llModel->stateSize();
}

uint64_t llmodel_save_state_data(llmodel_model model, uint8_t *dest)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    return wrapper->llModel->saveState(dest);
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

    if (size_t(ctx->n_past) < wrapper->promptContext.tokens.size())
        wrapper->promptContext.tokens.resize(ctx->n_past);

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

float *llmodel_embedding(llmodel_model model, const char *text, size_t *embedding_size)
{
    if (model == nullptr || text == nullptr || !strlen(text)) {
        *embedding_size = 0;
        return nullptr;
    }
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    std::vector<float> embeddingVector = wrapper->llModel->embedding(text);
    float *embedding = (float *)malloc(embeddingVector.size() * sizeof(float));
    if (embedding == nullptr) {
        *embedding_size = 0;
        return nullptr;
    }
    std::copy(embeddingVector.begin(), embeddingVector.end(), embedding);
    *embedding_size = embeddingVector.size();
    return embedding;
}

void llmodel_free_embedding(float *ptr)
{
    free(ptr);
}

void llmodel_setThreadCount(llmodel_model model, int32_t n_threads)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    wrapper->llModel->setThreadCount(n_threads);
}

int32_t llmodel_threadCount(llmodel_model model)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    return wrapper->llModel->threadCount();
}

void llmodel_set_implementation_search_path(const char *path)
{
    LLModel::Implementation::setImplementationsSearchPath(path);
}

const char *llmodel_get_implementation_search_path()
{
    return LLModel::Implementation::implementationsSearchPath().c_str();
}
