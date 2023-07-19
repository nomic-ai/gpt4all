#ifndef BERT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#error This file is NOT meant to be included outside of bert.cpp. Doing so is DANGEROUS. Be sure to know what you are doing before proceeding to #define BERT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#endif
#ifndef BERT_H
#define BERT_H

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include "llmodel.h"

struct BertPrivate;
class Bert : public LLModel {
public:
    Bert();
    ~Bert();

    bool supportsEmbedding() const override { return true; }
    bool supportsCompletion() const override { return true; }
    bool loadModel(const std::string &modelPath) override;
    bool isModelLoaded() const override;
    size_t requiredMem(const std::string &modelPath) override;
    size_t stateSize() const override;
    size_t saveState(uint8_t *dest) const override;
    size_t restoreState(const uint8_t *src) override;
    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() const override;

    std::vector<float> embedding(const std::string &text) override;

private:
    std::unique_ptr<BertPrivate> d_ptr;

protected:
    std::vector<Token> tokenize(PromptContext &, const std::string&) const override;
    Token sampleToken(PromptContext &ctx) const override;
    std::string tokenToString(Token) const override;
    bool evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const override;
    int32_t contextLength() const override;
    const std::vector<Token>& endTokens() const override;
};

#endif // BERT_H
