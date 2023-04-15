#ifndef LLAMAMODEL_H
#define LLAMAMODEL_H

#include <string>
#include <functional>
#include <vector>
#include "llmodel.h"

class LLamaPrivate;
class LLamaModel : public LLModel {
public:
    LLamaModel();
    ~LLamaModel();

    bool loadModel(const std::string &modelPath) override;
    bool loadModel(const std::string &modelPath, std::istream &fin) override;
    bool isModelLoaded() const override;
    void prompt(const std::string &prompt, std::function<bool(const std::string&)> response,
        PromptContext &ctx, int32_t n_predict = 200, int32_t top_k = 50400, float top_p = 1.0f,
        float temp = 0.0f, int32_t n_batch = 9) override;
    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() override;

private:
    LLamaPrivate *d_ptr;
};

#endif // LLAMAMODEL_H