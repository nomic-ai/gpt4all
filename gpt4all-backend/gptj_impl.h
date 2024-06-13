#ifndef GPTJ_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#error This file is NOT meant to be included outside of gptj.cpp. Doing so is DANGEROUS. Be sure to know what you are doing before proceeding to #define GPTJ_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#endif
#ifndef GPTJ_H
#define GPTJ_H

#include "llmodel.h"

#include <functional>
#include <string>
#include <vector>

struct GPTJPrivate;
class GPTJ : public LLModel {
public:
    GPTJ();
    ~GPTJ();

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

private:
    GPTJPrivate *d_ptr;

protected:
    std::vector<Token> tokenize(PromptContext &ctx, const std::string &str, bool special) const override;
    Token sampleToken(PromptContext &ctx) const override;
    std::string tokenToString(Token id) const override;
    bool evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const override;
    int32_t contextLength() const override;
    const std::vector<Token> &endTokens() const override;
    bool shouldAddBOS() const override { return false; }
};

#endif // GPTJ_H
