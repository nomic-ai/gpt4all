#ifndef LLMODEL_H
#define LLMODEL_H

#include <string>
#include <functional>
#include <vector>
#include <cstdint>

class LLModel {
public:
    explicit LLModel() {}
    virtual ~LLModel() {}

    virtual bool loadModel(const std::string &modelPath) = 0;
    virtual bool isModelLoaded() const = 0;
    virtual size_t stateSize() const { return 0; }
    virtual size_t saveState(uint8_t *dest) const { return 0; }
    virtual size_t restoreState(const uint8_t *src) { return 0; }
    struct PromptContext {
        std::vector<float> logits;      // logits of current context
        std::vector<int32_t> tokens;    // current tokens in the context window
        int32_t n_past = 0;             // number of tokens in past conversation
        int32_t n_ctx = 0;              // number of tokens possible in context window
        int32_t n_predict = 200;
        int32_t top_k = 40;
        float   top_p = 0.9f;
        float   temp = 0.9f;
        int32_t n_batch = 9;
        float   repeat_penalty = 1.10f;
        int32_t repeat_last_n = 64;     // last n tokens to penalize
        float   contextErase = 0.75f;   // percent of context to erase if we exceed the context
                                        // window
    };
    virtual void prompt(const std::string &prompt,
        std::function<bool(int32_t)> promptCallback,
        std::function<bool(int32_t, const std::string&)> responseCallback,
        std::function<bool(bool)> recalculateCallback,
        PromptContext &ctx) = 0;
    virtual void setThreadCount(int32_t n_threads) {}
    virtual int32_t threadCount() const { return 1; }

protected:
    virtual void recalculateContext(PromptContext &promptCtx,
        std::function<bool(bool)> recalculate) = 0;
};

#endif // LLMODEL_H
