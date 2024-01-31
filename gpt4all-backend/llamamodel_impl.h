#ifndef LLAMAMODEL_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#error This file is NOT meant to be included outside of llamamodel.cpp. Doing so is DANGEROUS. Be sure to know what you are doing before proceeding to #define LLAMAMODEL_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#endif
#ifndef LLAMAMODEL_H
#define LLAMAMODEL_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "llmodel.h"

struct LLamaPrivate;
class LLamaModel : public LLModel {
public:
    LLamaModel();
    ~LLamaModel();

    bool supportsEmbedding() const override { return false; }
    bool supportsCompletion() const override { return true; }
    bool loadModel(const std::string &modelPath, int n_ctx, int ngl) override;
    bool isModelLoaded() const override;
    size_t requiredMem(const std::string &modelPath, int n_ctx, int ngl) override;
    size_t stateSize() const override;
    size_t saveState(uint8_t *dest) const override;
    size_t restoreState(const uint8_t *src) override;
    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() const override;
    std::vector<GPUDevice> availableGPUDevices(size_t memoryRequired) const override;
    bool initializeGPUDevice(size_t memoryRequired, const std::string& name) const override;
    bool initializeGPUDevice(int device, std::string *unavail_reason) const override;
    bool hasGPUDevice() override;
    bool usingGPUDevice() override;

private:
    std::unique_ptr<LLamaPrivate> d_ptr;

protected:
    std::vector<Token> tokenize(PromptContext &, const std::string&) const override;
    std::string tokenToString(Token) const override;
    Token sampleToken(PromptContext& ctx) const override;
    bool evalTokens(PromptContext& ctx, const std::vector<int32_t> &tokens) const override;
    int32_t contextLength() const override;
    const std::vector<Token>& endTokens() const override;

    int32_t maxContextLength(std::string const &modelPath) const override;
    int32_t layerCount(std::string const &modelPath) const override;
};

#endif // LLAMAMODEL_H
