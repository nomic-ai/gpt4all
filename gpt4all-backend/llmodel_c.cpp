#include "llmodel_c.h"
#include "llmodel.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <optional>
#include <utility>

struct LLModelWrapper {
    LLModel *llModel = nullptr;
    LLModel::PromptContext promptContext;
    ~LLModelWrapper() { delete llModel; }
};

llmodel_model llmodel_model_create(const char *model_path) {
    const char *error;
    auto fres = llmodel_model_create2(model_path, "auto", &error);
    if (!fres) {
        fprintf(stderr, "Unable to instantiate model: %s\n", error);
    }
    return fres;
}

static void llmodel_set_error(const char **errptr, const char *message) {
    thread_local static std::string last_error_message;
    if (errptr) {
        last_error_message = message;
        *errptr = last_error_message.c_str();
    }
}

llmodel_model llmodel_model_create2(const char *model_path, const char *build_variant, const char **error) {
    LLModel *llModel;
    try {
        llModel = LLModel::Implementation::construct(model_path, build_variant);
    } catch (const std::exception& e) {
        llmodel_set_error(error, e.what());
        return nullptr;
    }

    if (!llModel) {
        llmodel_set_error(error, "Model format not supported (no matching implementation found)");
        return nullptr;
    }

    auto wrapper = new LLModelWrapper;
    wrapper->llModel = llModel;
    return wrapper;
}

void llmodel_model_destroy(llmodel_model model) {
    delete static_cast<LLModelWrapper *>(model);
}

size_t llmodel_required_mem(llmodel_model model, const char *model_path, int n_ctx, int ngl)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->requiredMem(model_path, n_ctx, ngl);
}

bool llmodel_loadModel(llmodel_model model, const char *model_path, int n_ctx, int ngl)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);

    std::string modelPath(model_path);
    if (wrapper->llModel->isModelBlacklisted(modelPath)) {
        size_t slash = modelPath.find_last_of("/\\");
        auto basename = slash == std::string::npos ? modelPath : modelPath.substr(slash + 1);
        std::cerr << "warning: model '" << basename << "' is out-of-date, please check for an updated version\n";
    }
    return wrapper->llModel->loadModel(modelPath, n_ctx, ngl);
}

bool llmodel_isModelLoaded(llmodel_model model)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->isModelLoaded();
}

uint64_t llmodel_get_state_size(llmodel_model model)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->stateSize();
}

uint64_t llmodel_save_state_data(llmodel_model model, uint8_t *dest)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->saveState(dest);
}

uint64_t llmodel_restore_state_data(llmodel_model model, const uint8_t *src)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->restoreState(src);
}

void llmodel_prompt(llmodel_model model, const char *prompt,
                    const char *prompt_template,
                    llmodel_prompt_callback prompt_callback,
                    llmodel_response_callback response_callback,
                    llmodel_recalculate_callback recalculate_callback,
                    llmodel_prompt_context *ctx,
                    bool special,
                    const char *fake_reply)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);

    auto response_func = [response_callback](int32_t token_id, const std::string &response) {
        return response_callback(token_id, response.c_str());
    };

    if (size_t(ctx->n_past) < wrapper->promptContext.tokens.size())
        wrapper->promptContext.tokens.resize(ctx->n_past);

    // Copy the C prompt context
    wrapper->promptContext.n_past = ctx->n_past;
    wrapper->promptContext.n_ctx = ctx->n_ctx;
    wrapper->promptContext.n_predict = ctx->n_predict;
    wrapper->promptContext.top_k = ctx->top_k;
    wrapper->promptContext.top_p = ctx->top_p;
    wrapper->promptContext.min_p = ctx->min_p;
    wrapper->promptContext.temp = ctx->temp;
    wrapper->promptContext.n_batch = ctx->n_batch;
    wrapper->promptContext.repeat_penalty = ctx->repeat_penalty;
    wrapper->promptContext.repeat_last_n = ctx->repeat_last_n;
    wrapper->promptContext.contextErase = ctx->context_erase;

    std::string fake_reply_str;
    if (fake_reply) { fake_reply_str = fake_reply; }
    auto *fake_reply_p = fake_reply ? &fake_reply_str : nullptr;

    // Call the C++ prompt method
    wrapper->llModel->prompt(prompt, prompt_template, prompt_callback, response_func, recalculate_callback,
                             wrapper->promptContext, special, fake_reply_p);

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
    ctx->min_p = wrapper->promptContext.min_p;
    ctx->temp = wrapper->promptContext.temp;
    ctx->n_batch = wrapper->promptContext.n_batch;
    ctx->repeat_penalty = wrapper->promptContext.repeat_penalty;
    ctx->repeat_last_n = wrapper->promptContext.repeat_last_n;
    ctx->context_erase = wrapper->promptContext.contextErase;
}

float *llmodel_embed(
    llmodel_model model, const char **texts, size_t *embedding_size, const char *prefix, int dimensionality,
    size_t *token_count, bool do_mean, bool atlas, const char **error
) {
    auto *wrapper = static_cast<LLModelWrapper *>(model);

    if (!texts || !*texts) {
        llmodel_set_error(error, "'texts' is NULL or empty");
        return nullptr;
    }

    std::vector<std::string> textsVec;
    while (*texts) { textsVec.emplace_back(*texts++); }

    size_t embd_size;
    float *embedding;

    try {
        embd_size = wrapper->llModel->embeddingSize();
        if (dimensionality > 0 && dimensionality < int(embd_size))
            embd_size = dimensionality;

        embd_size *= textsVec.size();

        std::optional<std::string> prefixStr;
        if (prefix) { prefixStr = prefix; }

        embedding = new float[embd_size];
        wrapper->llModel->embed(textsVec, embedding, prefixStr, dimensionality, token_count, do_mean, atlas);
    } catch (std::exception const &e) {
        llmodel_set_error(error, e.what());
        return nullptr;
    }

    *embedding_size = embd_size;
    return embedding;
}

void llmodel_free_embedding(float *ptr)
{
    delete[] ptr;
}

void llmodel_setThreadCount(llmodel_model model, int32_t n_threads)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    wrapper->llModel->setThreadCount(n_threads);
}

int32_t llmodel_threadCount(llmodel_model model)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
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

struct llmodel_gpu_device* llmodel_available_gpu_devices(llmodel_model model, size_t memoryRequired, int* num_devices)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    std::vector<LLModel::GPUDevice> devices = wrapper->llModel->availableGPUDevices(memoryRequired);

    // Set the num_devices
    *num_devices = devices.size();

    if (*num_devices == 0) return nullptr;  // Return nullptr if no devices are found

    // Allocate memory for the output array
    struct llmodel_gpu_device* output = (struct llmodel_gpu_device*) malloc(*num_devices * sizeof(struct llmodel_gpu_device));

    for (int i = 0; i < *num_devices; i++) {
        output[i].index = devices[i].index;
        output[i].type = devices[i].type;
        output[i].heapSize = devices[i].heapSize;
        output[i].name = strdup(devices[i].name.c_str());  // Convert std::string to char* and allocate memory
        output[i].vendor = strdup(devices[i].vendor.c_str());  // Convert std::string to char* and allocate memory
    }

    return output;
}

bool llmodel_gpu_init_gpu_device_by_string(llmodel_model model, size_t memoryRequired, const char *device)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->initializeGPUDevice(memoryRequired, std::string(device));
}

bool llmodel_gpu_init_gpu_device_by_struct(llmodel_model model, const llmodel_gpu_device *device)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->initializeGPUDevice(device->index);
}

bool llmodel_gpu_init_gpu_device_by_int(llmodel_model model, int device)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->initializeGPUDevice(device);
}

bool llmodel_has_gpu_device(llmodel_model model)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->hasGPUDevice();
}
