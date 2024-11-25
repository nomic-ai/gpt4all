#include "llmodel_c.h"

#include "llmodel.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace ranges = std::ranges;

static_assert(sizeof(token_t) == sizeof(LLModel::Token));

struct LLModelWrapper {
    LLModel *llModel = nullptr;
    ~LLModelWrapper() { delete llModel; }
};

llmodel_model llmodel_model_create(const char *model_path)
{
    const char *error;
    auto fres = llmodel_model_create2(model_path, "auto", &error);
    if (!fres) {
        fprintf(stderr, "Unable to instantiate model: %s\n", error);
    }
    return fres;
}

static void llmodel_set_error(const char **errptr, const char *message)
{
    thread_local static std::string last_error_message;
    if (errptr) {
        last_error_message = message;
        *errptr = last_error_message.c_str();
    }
}

llmodel_model llmodel_model_create2(const char *model_path, const char *backend, const char **error)
{
    LLModel *llModel;
    try {
        llModel = LLModel::Implementation::construct(model_path, backend);
    } catch (const std::exception& e) {
        llmodel_set_error(error, e.what());
        return nullptr;
    }

    auto wrapper = new LLModelWrapper;
    wrapper->llModel = llModel;
    return wrapper;
}

void llmodel_model_destroy(llmodel_model model)
{
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

uint64_t llmodel_state_get_size(llmodel_model model)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->stateSize();
}

uint64_t llmodel_state_get_data(llmodel_model model, uint8_t *state_out, uint64_t state_size,
                                token_t **input_tokens_out, uint64_t *n_input_tokens)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    std::vector<LLModel::Token> inputTokens;
    auto bytesWritten = wrapper->llModel->saveState({state_out, size_t(state_size)}, inputTokens);
    if (bytesWritten) {
        auto *buf = new LLModel::Token[inputTokens.size()];
        ranges::copy(inputTokens, buf);
        *input_tokens_out = buf;
        *n_input_tokens = uint64_t(inputTokens.size());
    } else {
        *input_tokens_out = nullptr;
        *n_input_tokens = 0;
    }
    return bytesWritten;
}

void llmodel_state_free_input_tokens(LLModel::Token *input_tokens)
{
    delete[] input_tokens;
}

uint64_t llmodel_state_set_data(llmodel_model model, const uint8_t *state, uint64_t state_size,
                                const token_t *input_tokens, uint64_t n_input_tokens)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->restoreState({state, size_t(state_size)}, {input_tokens, size_t(n_input_tokens)});
}

bool llmodel_prompt(llmodel_model               model,
                    const char                 *prompt,
                    llmodel_prompt_callback     prompt_callback,
                    llmodel_response_callback   response_callback,
                    llmodel_prompt_context     *ctx,
                    const char                **error)
{
    auto *wrapper = static_cast<LLModelWrapper *>(model);

    // Copy the C prompt context
    LLModel::PromptContext promptContext {
        .n_predict      = ctx->n_predict,
        .top_k          = ctx->top_k,
        .top_p          = ctx->top_p,
        .min_p          = ctx->min_p,
        .temp           = ctx->temp,
        .n_batch        = ctx->n_batch,
        .repeat_penalty = ctx->repeat_penalty,
        .repeat_last_n  = ctx->repeat_last_n,
        .contextErase   = ctx->context_erase,
    };

    auto prompt_func = [prompt_callback](std::span<const LLModel::Token> token_ids, bool cached) {
        return prompt_callback(token_ids.data(), token_ids.size(), cached);
    };
    auto response_func = [response_callback](LLModel::Token token_id, std::string_view piece) {
        return response_callback(token_id, piece.data());
    };

    // Call the C++ prompt method
    try {
        wrapper->llModel->prompt(prompt, prompt_func, response_func, promptContext);
    } catch (std::exception const &e) {
        llmodel_set_error(error, e.what());
        return false;
    }

    return true;
}

float *llmodel_embed(
    llmodel_model model, const char **texts, size_t *embedding_size, const char *prefix, int dimensionality,
    size_t *token_count, bool do_mean, bool atlas, llmodel_emb_cancel_callback cancel_cb, const char **error
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
        wrapper->llModel->embed(textsVec, embedding, prefixStr, dimensionality, token_count, do_mean, atlas, cancel_cb);
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

// RAII wrapper around a C-style struct
struct llmodel_gpu_device_cpp: llmodel_gpu_device {
    llmodel_gpu_device_cpp() = default;

    llmodel_gpu_device_cpp(const llmodel_gpu_device_cpp  &) = delete;
    llmodel_gpu_device_cpp(      llmodel_gpu_device_cpp &&) = delete;

    const llmodel_gpu_device_cpp &operator=(const llmodel_gpu_device_cpp  &) = delete;
          llmodel_gpu_device_cpp &operator=(      llmodel_gpu_device_cpp &&) = delete;

    ~llmodel_gpu_device_cpp() {
        free(const_cast<char *>(name));
        free(const_cast<char *>(vendor));
    }
};

static_assert(sizeof(llmodel_gpu_device_cpp) == sizeof(llmodel_gpu_device));

struct llmodel_gpu_device *llmodel_available_gpu_devices(size_t memoryRequired, int *num_devices)
{
    static thread_local std::unique_ptr<llmodel_gpu_device_cpp[]> c_devices;

    auto devices = LLModel::Implementation::availableGPUDevices(memoryRequired);
    *num_devices = devices.size();

    if (devices.empty()) { return nullptr; /* no devices */ }

    c_devices = std::make_unique<llmodel_gpu_device_cpp[]>(devices.size());
    for (unsigned i = 0; i < devices.size(); i++) {
        const auto &dev  =   devices[i];
              auto &cdev = c_devices[i];
        cdev.backend  = dev.backend;
        cdev.index    = dev.index;
        cdev.type     = dev.type;
        cdev.heapSize = dev.heapSize;
        cdev.name     = strdup(dev.name.c_str());
        cdev.vendor   = strdup(dev.vendor.c_str());
    }

    return c_devices.get();
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

const char *llmodel_model_backend_name(llmodel_model model)
{
    const auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->backendName();
}

const char *llmodel_model_gpu_device_name(llmodel_model model)
{
    const auto *wrapper = static_cast<LLModelWrapper *>(model);
    return wrapper->llModel->gpuDeviceName();
}

int32_t llmodel_count_prompt_tokens(llmodel_model model, const char *prompt, const char **error)
{
    auto *wrapper = static_cast<const LLModelWrapper *>(model);
    try {
        return wrapper->llModel->countPromptTokens(prompt);
    } catch (const std::exception& e) {
        llmodel_set_error(error, e.what());
        return -1;
    }
}

void llmodel_model_foreach_special_token(llmodel_model model, llmodel_special_token_callback callback)
{
    auto *wrapper = static_cast<const LLModelWrapper *>(model);
    for (auto &[name, token] : wrapper->llModel->specialTokens())
        callback(name.c_str(), token.c_str());
}
