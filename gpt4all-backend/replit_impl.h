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

class ReplitPrivate;
class Replit : public LLModel {
public:
    Replit();
    ~Replit();

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
    ReplitPrivate *d_ptr;
};

#endif // REPLIT_H
