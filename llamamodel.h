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
    void prompt(const std::string &prompt,
        std::function<bool(int32_t, const std::string&)> response,
        PromptContext &ctx) override;
    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() override;

private:
    LLamaPrivate *d_ptr;
};

#endif // LLAMAMODEL_H