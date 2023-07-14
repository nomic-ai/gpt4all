#ifndef REPLIT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#error This file is NOT meant to be included outside of replit.cpp. Doing so is DANGEROUS. Be sure to know what you are doing before proceeding to #define REPLIT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#endif
#ifndef REPLIT_H
#define REPLIT_H

#include <string>
#include <functional>
#include <vector>
#include "llmodel.h"

#define GGML_QNT_VERSION_FACTOR 1000 // do not change this

struct ReplitPrivate;
class Replit : public LLModel {
public:
    Replit();
    ~Replit();

    bool supportsEmbedding() const override { return false; }
    bool supportsCompletion() const override { return true; }
    bool loadModel(const std::string &modelPath) override;
    bool isModelLoaded() const override;
    size_t requiredMem(const std::string & modelPath) override;
    size_t stateSize() const override;
    size_t saveState(uint8_t *dest) const override;
    size_t restoreState(const uint8_t *src) override;
    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() const override;

private:
    ReplitPrivate *d_ptr;

protected:
    std::vector<Token> tokenize(PromptContext &, const std::string&) const override;
    std::string tokenToString(Token) const override;
    Token sampleToken(PromptContext &ctx) const override;
    bool evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const override;
    int32_t contextLength() const override;
    const std::vector<Token>& endTokens() const override;
};

#endif // REPLIT_H
