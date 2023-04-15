#ifndef LLMODEL_H
#define LLMODEL_H

#include <string>
#include <functional>
#include <vector>

class LLModel {
public:
    explicit LLModel() {}
    virtual ~LLModel() {}

    virtual bool loadModel(const std::string &modelPath) = 0;
    virtual bool loadModel(const std::string &modelPath, std::istream &fin) = 0;
    virtual bool isModelLoaded() const = 0;
    struct PromptContext {
        std::vector<float> logits;
        int32_t n_past = 0; // number of tokens in past conversation
    };
    virtual void prompt(const std::string &prompt, std::function<bool(const std::string&)> response,
        PromptContext &ctx, int32_t n_predict = 200, int32_t top_k = 40, float top_p = 0.9f,
        float temp = 0.9f, int32_t n_batch = 9) = 0;
    virtual void setThreadCount(int32_t n_threads) {}
    virtual int32_t threadCount() { return 1; }
};

#endif // LLMODEL_H
