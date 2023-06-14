#include "llmodel_c.h"
#include "llmodel.h"

#include <cstring>
#include <cerrno>
#include <string>
#include <string_view>
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

bool prompt_wrapper2(std::string_view token, float logit, bool isPartOfTemplate, void *user_data) {
    llmodel_prompt_callback2 callback = reinterpret_cast<llmodel_prompt_callback2>(user_data);
    return callback(token.data()/*Potentially unsafe*/, logit, isPartOfTemplate);
}

bool response_wrapper2(std::string_view token, float logit, void *user_data) {
    llmodel_response_callback2 callback = reinterpret_cast<llmodel_response_callback2>(user_data);
    return callback(token.data()/*Potentially unsafe*/, logit);
}

bool recalculate_wrapper2(bool is_recalculating, void *user_data) {
    llmodel_recalculate_callback2 callback = reinterpret_cast<llmodel_recalculate_callback2>(user_data);
    return callback(is_recalculating);
}

static void CToCppPromptContext(llmodel_prompt_context *input, LLModel::PromptContext& output) {
    output.n_past = input->n_past;
    output.n_ctx = input->n_ctx;
    output.n_predict = input->n_predict;
    output.top_k = input->top_k;
    output.top_p = input->top_p;
    output.temp = input->temp;
    output.n_batch = input->n_batch;
    output.repeat_penalty = input->repeat_penalty;
    output.repeat_last_n = input->repeat_last_n;
    output.contextErase = input->context_erase;
    std::unordered_set<std::string_view> cppReversePrompts;
    for (const char **reversePrompt = input->reversePrompts; *reversePrompt; reversePrompt++) {
        cppReversePrompts.emplace(*reversePrompt);
    }
}
static void CppToCPromptContext(LLModel::PromptContext &input, llmodel_prompt_context *output) {
    output->n_past = input.n_past;
    output->n_ctx = input.n_ctx;
    output->n_predict = input.n_predict;
    output->top_k = input.top_k;
    output->top_p = input.top_p;
    output->temp = input.temp;
    output->n_batch = input.n_batch;
    output->repeat_penalty = input.repeat_penalty;
    output->repeat_last_n = input.repeat_last_n;
    output->context_erase = input.contextErase;
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
    CToCppPromptContext(ctx, wrapper->promptContext);

    // Call the C++ prompt method
    wrapper->llModel->prompt(prompt, prompt_func, response_func, recalc_func, wrapper->promptContext);

    // Update the C context by giving access to the wrappers raw pointers to std::vector data
    // which involves no copies
    ctx->logits = wrapper->promptContext.logits.data();
    ctx->logits_size = wrapper->promptContext.logits.size();
    ctx->tokens = wrapper->promptContext.tokens.data();
    ctx->tokens_size = wrapper->promptContext.tokens.size();

    // Update the rest of the C prompt context
    CppToCPromptContext(wrapper->promptContext, ctx);
}

bool llmodel_prompt2(llmodel_model model, const char *prompt,
                     const char *template_prefix, const char *template_suffix,
                     llmodel_prompt_callback2 prompt_callback,
                     llmodel_response_callback2 response_callback,
                     llmodel_recalculate_callback2 recalculate_callback,
                     llmodel_prompt_context *ctx, llmodel_error *error) {
    LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper*>(model);
    bool fres = true;

    // Create std::function wrappers that call the C function pointers
    LLModel::PromptCallback prompt_func =
        std::bind(&prompt_wrapper2, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, reinterpret_cast<void*>(prompt_callback));
    LLModel::ResponseCallback response_func =
        std::bind(&response_wrapper2, std::placeholders::_1, std::placeholders::_2, reinterpret_cast<void*>(response_callback));
    LLModel::RecalculateCallback recalc_func =
        std::bind(&recalculate_wrapper2, std::placeholders::_1, reinterpret_cast<void*>(recalculate_callback));

    // Copy the C prompt context
    CToCppPromptContext(ctx, wrapper->promptContext);

    // Call the C++ prompt method
    try {
        wrapper->llModel->prompt(template_prefix, template_suffix, prompt, prompt_func, response_func, recalc_func, wrapper->promptContext);
    } catch (const LLModel::Exception& e) {
        error->code = -1;
        last_error_message = e.what();
        error->message = last_error_message.c_str();
        fres = false;
    }

    // Update the C context by giving access to the wrappers raw pointers to std::vector data
    // which involves no copies
    ctx->logits = wrapper->promptContext.logits.data();
    ctx->logits_size = wrapper->promptContext.logits.size();
    ctx->tokens = wrapper->promptContext.tokens.data();
    ctx->tokens_size = wrapper->promptContext.tokens.size();

    // Update the rest of the C prompt context
    CppToCPromptContext(wrapper->promptContext, ctx);

    return fres;
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
    LLModel::setImplementationsSearchPath(path);
}

const char *llmodel_get_implementation_search_path()
{
    return LLModel::implementationsSearchPath().c_str();
}

