#ifndef GPTJ_H
#define GPTJ_H

#include <string>
#include <functional>
#include <vector>
#include "llmodel.h"

class GPTJPrivate;
class GPTJ : public LLModel {
public:
    GPTJ();
    ~GPTJ();

    bool loadModel(const std::string &modelPath) override;
    bool isModelLoaded() const override;
    size_t stateSize() const override;
    size_t saveState(uint8_t *dest) const override;
    size_t restoreState(const uint8_t *src) override;
    void prompt(const std::string &prompt,
        std::function<bool(int32_t)> promptCallback,
        std::function<bool(int32_t, const std::string&)> responseCallback,
        std::function<bool(bool)> recalculateCallback,
        PromptContext &ctx) override;
    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() const override;

protected:
    void recalculateContext(PromptContext &promptCtx,
        std::function<bool(bool)> recalculate) override;

private:
    GPTJPrivate *d_ptr;
};

#endif // GPTJ_H
