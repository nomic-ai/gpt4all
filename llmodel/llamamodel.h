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
    bool isModelLoaded() const override;
    void prompt(const std::string &prompt,
        std::function<bool(int32_t)> promptCallback,
        std::function<bool(int32_t, const std::string&)> responseCallback,
        std::function<bool(bool)> recalculateCallback,
        PromptContext &ctx) override;
    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() override;

protected:
    void recalculateContext(PromptContext &promptCtx,
        std::function<bool(bool)> recalculate) override;

private:
    LLamaPrivate *d_ptr;
};

#endif // LLAMAMODEL_H