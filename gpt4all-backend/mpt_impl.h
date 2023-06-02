#ifndef MPT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#error This file is NOT meant to be included outside of mpt.cpp. Doing so is DANGEROUS. Be sure to know what you are doing before proceeding to #define MPT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#endif
#ifndef MPT_H
#define MPT_H

#include <string>
#include <functional>
#include <vector>
#include "llmodel.h"

struct MPTPrivate;
class MPT : public LLModel {
public:
    MPT();
    ~MPT();

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
    MPTPrivate *d_ptr;
};

#endif // MPT_H
