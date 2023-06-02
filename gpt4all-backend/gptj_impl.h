#ifndef GPTJ_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#error This file is NOT meant to be included outside of gptj.cpp. Doing so is DANGEROUS. Be sure to know what you are doing before proceeding to #define GPTJ_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#endif
#ifndef GPTJ_H
#define GPTJ_H

#include <string>
#include <functional>
#include <vector>
#include "llmodel.h"

struct GPTJPrivate;
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
    bool evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) override;
    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() const override;

private:
    GPTJPrivate *d_ptr;
};

#endif // GPTJ_H
