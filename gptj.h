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

    bool loadModel(const std::string &modelPath, std::istream &fin) override;
    bool isModelLoaded() const override;
    void prompt(const std::string &prompt, std::function<bool(const std::string&)> response,
        PromptContext &ctx, int32_t n_predict = 200, int32_t top_k = 50400, float top_p = 1.0f,
        float temp = 0.0f, int32_t n_batch = 9) override;

private:
    GPTJPrivate *d_ptr;
};

#endif // GPTJ_H