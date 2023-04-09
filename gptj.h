#ifndef GPTJ_H
#define GPTJ_H

#include <string>
#include <sstream>
#include <functional>

class GPTJPrivate;
class GPTJ {
public:
    GPTJ();
    ~GPTJ();

    bool loadModel(const std::string &modelPath, std::istream &fin);
    bool isModelLoaded() const;
    struct PromptContext {
        std::vector<float> logits;
        int32_t n_past = 0; // number of tokens in past conversation
    };
    void prompt(const std::string &prompt, std::function<bool(const std::string&)> response,
        PromptContext &ctx, int32_t n_predict = 200, int32_t top_k = 40, float top_p = 0.9f,
        float temp = 0.9f, int32_t n_batch = 9);

private:
    GPTJPrivate *d_ptr;
};

#endif // GPTJ_H