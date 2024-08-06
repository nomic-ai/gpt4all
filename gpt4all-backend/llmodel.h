#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

class Dlhandle;

using namespace std::string_literals;

#define LLMODEL_MAX_PROMPT_BATCH 128

class LLModel {
public:
    using Token = int32_t;

    struct PromptContext {
        std::vector<int32_t> tokens;    // current tokens in the context window
        int32_t n_past = 0;             // number of tokens in past conversation
        int32_t n_ctx = 0;              // number of tokens possible in context window
        int32_t n_predict = 200;
        int32_t top_k = 40;
        float   top_p = 0.9f;
        float   min_p = 0.0f;
        float   temp = 0.9f;
        int32_t n_batch = 9;
        float   repeat_penalty = 1.10f;
        int32_t repeat_last_n = 64;     // last n tokens to penalize
        float   contextErase = 0.5f;    // percent of context to erase if we exceed the context window
    };

    virtual ~LLModel() {}

    virtual bool supportsCompletion() const { return true; }
    virtual bool loadModel(const std::string &modelPath, int n_ctx, int ngl) = 0;
    virtual bool isModelLoaded() const = 0;
    virtual size_t stateSize() const { return 0; }
    virtual size_t saveState(uint8_t *dest) const { (void)dest; return 0; }
    virtual size_t restoreState(const uint8_t *src) { (void)src; return 0; }

    // This method requires the model to return true from supportsCompletion otherwise it will throw
    // an error
    virtual void prompt(const std::string &prompt,
                        const std::string &promptTemplate,
                        std::function<bool(int32_t)> promptCallback,
                        std::function<bool(int32_t, const std::string&)> responseCallback,
                        bool allowContextShift,
                        PromptContext &ctx,
                        bool special = false,
                        std::string *fakeReply = nullptr) = 0;

protected:
    explicit LLModel() {}
};

class EmbLLModel: virtual public LLModel {
public:
    using EmbedCancelCallback = bool(unsigned *batchSizes, unsigned nBatch, const char *backend);

    virtual bool supportsCompletion() const = 0;
    virtual bool supportsEmbedding() const = 0;
    virtual size_t embeddingSize() const = 0;

    // user-specified prefix
    virtual void embed(const std::vector<std::string> &texts, float *embeddings, std::optional<std::string> prefix,
                       int dimensionality = -1, size_t *tokenCount = nullptr, bool doMean = true, bool atlas = false,
                       EmbedCancelCallback *cancelCb = nullptr) = 0;
    // automatic prefix
    virtual void embed(const std::vector<std::string> &texts, float *embeddings, bool isRetrieval,
                       int dimensionality = -1, size_t *tokenCount = nullptr, bool doMean = true, bool atlas = false) = 0;
};
