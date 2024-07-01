#ifndef LLAMAMODEL_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#error This file is NOT meant to be included outside of llamamodel.cpp. Doing so is DANGEROUS. Be sure to know what you are doing before proceeding to #define LLAMAMODEL_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#endif
#ifndef LLAMAMODEL_H
#define LLAMAMODEL_H

#include "llmodel.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct LLamaPrivate;
struct EmbModelSpec;

class LLamaModel : public LLModel {
public:
    LLamaModel();
    ~LLamaModel();

    bool supportsEmbedding() const override { return m_supportsEmbedding; }
    bool supportsCompletion() const override { return m_supportsCompletion; }
    bool loadModel(const std::string &modelPath, int n_ctx, int ngl) override;
    bool isModelBlacklisted(const std::string &modelPath) const override;
    bool isEmbeddingModel(const std::string &modelPath) const override;
    bool isModelLoaded() const override;
    size_t requiredMem(const std::string &modelPath, int n_ctx, int ngl) override;
    size_t stateSize() const override;
    size_t saveState(uint8_t *dest) const override;
    size_t restoreState(const uint8_t *src) override;
    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() const override;
    std::vector<GPUDevice> availableGPUDevices(size_t memoryRequired = 0) const override;
    bool initializeGPUDevice(size_t memoryRequired, const std::string &name) const override;
    bool initializeGPUDevice(int device, std::string *unavail_reason = nullptr) const override;
    bool usingGPUDevice() const override;
    const char *backendName() const override;
    const char *gpuDeviceName() const override;

    size_t embeddingSize() const override;
    // user-specified prefix
    void embed(const std::vector<std::string> &texts, float *embeddings, std::optional<std::string> prefix,
               int dimensionality = -1, size_t *tokenCount = nullptr, bool doMean = true, bool atlas = false,
               EmbedCancelCallback *cancelCb = nullptr) override;
    // automatic prefix
    void embed(const std::vector<std::string> &texts, float *embeddings, bool isRetrieval, int dimensionality = -1,
               size_t *tokenCount = nullptr, bool doMean = true, bool atlas = false) override;

private:
    std::unique_ptr<LLamaPrivate> d_ptr;
    bool m_supportsEmbedding = false;
    bool m_supportsCompletion = false;

protected:
    std::vector<Token> tokenize(PromptContext &ctx, const std::string &str, bool special) const override;
    std::string tokenToString(Token id) const override;
    Token sampleToken(PromptContext &ctx) const override;
    bool evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const override;
    int32_t contextLength() const override;
    const std::vector<Token> &endTokens() const override;
    bool shouldAddBOS() const override;
    int32_t maxContextLength(std::string const &modelPath) const override;
    int32_t layerCount(std::string const &modelPath) const override;

    void embedInternal(const std::vector<std::string> &texts, float *embeddings, std::string prefix, int dimensionality,
                       size_t *tokenCount, bool doMean, bool atlas, EmbedCancelCallback *cancelCb,
                       const EmbModelSpec *spec);
};

#endif // LLAMAMODEL_H
