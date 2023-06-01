#include "llmodel_c.h"
#include "llmodel.h"

#include <cstring>
#include <cerrno>
#include <utility>


struct LLModelWrapper {
    LLModel *llModel = nullptr;
    LLModel::PromptContext promptContext;
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
    llmodel_error new_error{};

    try {
        wrapper->llModel = LLModel::construct(model_path, build_variant);
    } catch (const std::exception& e) {
        new_error.code = EINVAL;
        last_error_message = e.what();
    }

    if (!wrapper->llModel) {
        delete std::exchange(wrapper, nullptr);
        // Get errno and error message if none
        if (new_error.code == 0) {
            new_error.code = errno;
            last_error_message = strerror(errno);
        }
        // Set message pointer
        new_error.message = last_error_message.c_str();
        // Set error argument
        if (error) *error = new_error;
    }
    return reinterpret_cast<llmodel_model*>(wrapper);
}

void llmodel_model_destroy(llmodel_model model) {
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    delete wrapper->llModel;
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

// Helper function for the C callbacks to update stop value according to legacy behavior value
void update_callbacks_stop_value(llmodel_prompt_callbacks *callbacks, bool should_stop) {
    *callbacks->stop = uint8_t(!callbacks->legacy_behavior)*(*callbacks->stop) + should_stop;
}

// Wrapper functions for the C callbacks
void prompt_wrapper(int32_t token_id, void *user_data) {
    llmodel_prompt_callbacks *callbacks = reinterpret_cast<llmodel_prompt_callbacks*>(user_data);
    bool should_stop = !callbacks->prompt_callback(token_id);
    update_callbacks_stop_value(callbacks, should_stop);
}

void prompt_progress_wrapper(float progress, void *user_data) {
    llmodel_prompt_callbacks *callbacks = reinterpret_cast<llmodel_prompt_callbacks*>(user_data);
    bool should_stop = !callbacks->prompt_progress_callback(progress);
    update_callbacks_stop_value(callbacks, should_stop);
}

void response_wrapper(int32_t token_id, const std::string &response, void *user_data) {
    llmodel_prompt_callbacks *callbacks = reinterpret_cast<llmodel_prompt_callbacks*>(user_data);
    bool should_stop = !callbacks->response_callback(token_id, response.c_str());
    update_callbacks_stop_value(callbacks, should_stop);
}

void recalculate_wrapper(bool is_recalculating, void *user_data) {
    llmodel_prompt_callbacks *callbacks = reinterpret_cast<llmodel_prompt_callbacks*>(user_data);
    bool should_stop = !callbacks->recalculate_callback(is_recalculating);
    update_callbacks_stop_value(callbacks, should_stop);
}

void llmodel_prompt(llmodel_model model, const char *prompt,
                    llmodel_prompt_callback prompt_callback,
                    llmodel_response_callback response_callback,
                    llmodel_recalculate_callback recalculate_callback,
                    llmodel_prompt_context *ctx)
{
    llmodel_prompt_callbacks cbs{};
    cbs.prompt_callback = prompt_callback;
    cbs.response_callback = response_callback;
    cbs.recalculate_callback = recalculate_callback;
    cbs.legacy_behavior = 1; // Enable legacy behavior
    llmodel_prompt2(model, prompt, &cbs, ctx);
}

void llmodel_prompt2(llmodel_model model, const char *prompt,
                     llmodel_prompt_callbacks *callbacks,
                     llmodel_prompt_context *ctx)
{
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);

    // Create std::function wrappers that call the C function pointers
    LLModel::PromptCallbacks cpp_callbacks;
    cpp_callbacks.promptCallback =
        std::bind(&prompt_wrapper, std::placeholders::_1, reinterpret_cast<void*>(callbacks));
    cpp_callbacks.promptProgressCallback =
        std::bind(&prompt_progress_wrapper, std::placeholders::_1, reinterpret_cast<void*>(callbacks));
    cpp_callbacks.responseCallback =
        std::bind(&response_wrapper, std::placeholders::_1, std::placeholders::_2, reinterpret_cast<void*>(callbacks));
    cpp_callbacks.recalculateCallback =
        std::bind(&recalculate_wrapper, std::placeholders::_1, reinterpret_cast<void*>(callbacks));

    callbacks->stop = reinterpret_cast<uint8_t*>(&cpp_callbacks.should_stop); // Should be safe on little endian since uint8_t is always equal or lower size than bool

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
    wrapper->llModel->prompt(prompt, cpp_callbacks, wrapper->promptContext);

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
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    return wrapper->llModel->threadCount();
}
