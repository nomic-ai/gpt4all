#pragma once

#include "model_backend.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::string_literals;

class LlamaCppBackendManager;


class LlamaCppBackend : public EmbCapableBackend {
public:
    struct GPUDevice {
        const char *backend;
        int index;
        int type;
        size_t heapSize;
        std::string name;
        std::string vendor;

        GPUDevice(const char *backend, int index, int type, size_t heapSize, std::string name, std::string vendor):
            backend(backend), index(index), type(type), heapSize(heapSize), name(std::move(name)),
            vendor(std::move(vendor)) {}

        std::string selectionName() const
        {
            assert(backend == "cuda"s || backend == "kompute"s);
            return backendName() + ": " + name;
        }

        std::string backendName() const { return backendIdToName(backend); }

        static std::string backendIdToName(const std::string &backend) { return s_backendNames.at(backend); }

        static std::string updateSelectionName(const std::string &name) {
            if (name == "Auto" || name == "CPU" || name == "Metal")
                return name;
            auto it = std::find_if(s_backendNames.begin(), s_backendNames.end(), [&name](const auto &entry) {
                return name.starts_with(entry.second + ": ");
            });
            if (it != s_backendNames.end())
                return name;
            return "Vulkan: " + name; // previously, there were only Vulkan devices
        }

    private:
        static inline const std::unordered_map<std::string, std::string> s_backendNames {
            {"cpu", "CPU"}, {"metal", "Metal"}, {"cuda", "CUDA"}, {"kompute", "Vulkan"},
        };
    };

    using ProgressCallback = std::function<bool(float progress)>;

    virtual bool isModelBlacklisted(const std::string &modelPath) const = 0;
    virtual bool isEmbeddingModel(const std::string &modelPath) const = 0;
    virtual size_t requiredMem(const std::string &modelPath, int n_ctx, int ngl) = 0;

    void prompt(const std::string &prompt,
                const std::string &promptTemplate,
                std::function<bool(int32_t)> promptCallback,
                std::function<bool(int32_t, const std::string&)> responseCallback,
                bool allowContextShift,
                PromptContext &ctx,
                bool special = false,
                std::string *fakeReply = nullptr) override;

    virtual void setThreadCount(int32_t n_threads) { (void)n_threads; }
    virtual int32_t threadCount() const { return 1; }

    const LlamaCppBackendManager &manager() const;

    virtual std::vector<GPUDevice> availableGPUDevices(size_t memoryRequired) const
    {
        (void)memoryRequired;
        return {};
    }

    virtual bool initializeGPUDevice(size_t memoryRequired, const std::string &name) const
    {
        (void)memoryRequired;
        (void)name;
        return false;
    }

    virtual bool initializeGPUDevice(int device, std::string *unavail_reason = nullptr) const
    {
        (void)device;
        if (unavail_reason) {
            *unavail_reason = "model has no GPU support";
        }
        return false;
    }

    virtual bool usingGPUDevice() const { return false; }
    virtual const char *backendName() const { return "cpu"; }
    virtual const char *gpuDeviceName() const { return nullptr; }

    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }

protected:
    virtual std::vector<Token> tokenize(PromptContext &ctx, const std::string &str, bool special = false) = 0;
    virtual bool isSpecialToken(Token id) const = 0;
    virtual std::string tokenToString(Token id) const = 0;
    virtual Token sampleToken(PromptContext &ctx) const = 0;
    virtual bool evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const = 0;
    virtual void shiftContext(PromptContext &promptCtx) = 0;
    virtual int32_t contextLength() const = 0;
    virtual const std::vector<Token> &endTokens() const = 0;
    virtual bool shouldAddBOS() const = 0;

    virtual int32_t maxContextLength(std::string const &modelPath) const = 0;
    virtual int32_t layerCount(std::string const &modelPath) const = 0;

    static bool staticProgressCallback(float progress, void* ctx)
    {
        LlamaCppBackend *model = static_cast<LlamaCppBackend *>(ctx);
        if (model && model->m_progressCallback)
            return model->m_progressCallback(progress);
        return true;
    }

    bool decodePrompt(std::function<bool(int32_t)> promptCallback,
                      std::function<bool(int32_t, const std::string&)> responseCallback,
                      bool allowContextShift,
                      PromptContext &promptCtx,
                      std::vector<Token> embd_inp);
    void generateResponse(std::function<bool(int32_t, const std::string&)> responseCallback,
                          bool allowContextShift,
                          PromptContext &promptCtx);

    const LlamaCppBackendManager *m_manager = nullptr;
    ProgressCallback              m_progressCallback;
    Token                         m_tokenize_last_token = -1;

    friend class LlamaCppBackendManager;
};
