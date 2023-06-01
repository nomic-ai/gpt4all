#ifndef LLMODEL_H
#define LLMODEL_H

#include <string>
#include <functional>
#include <vector>
#include <string_view>
#include <fstream>
#include <cstdint>

class Dlhandle;

class LLModel {
public:
    class Implementation {
        LLModel *(*construct_)();

    public:
        Implementation(Dlhandle&&);
        Implementation(const Implementation&) = delete;
        Implementation(Implementation&&);
        ~Implementation();

        static bool isImplementation(const Dlhandle&);

        std::string_view modelType, buildVariant;
        bool (*magicMatch)(std::ifstream& f);
        Dlhandle *dlhandle;

        // The only way an implementation should be constructed
        LLModel *construct() const {
            auto fres = construct_();
            fres->m_implementation = this;
            return fres;
        }
    };

    struct PromptContext {
        std::vector<float> logits;      // logits of current context
        std::vector<int32_t> tokens;    // current tokens in the context window
        int32_t n_past = 0;             // number of tokens in past conversation
        int32_t n_ctx = 0;              // number of tokens possible in context window
        int32_t n_predict = 200;
        int32_t top_k = 40;
        float   top_p = 0.9f;
        float   temp = 0.9f;
        int32_t n_batch = 9;
        float   repeat_penalty = 1.10f;
        int32_t repeat_last_n = 64;     // last n tokens to penalize
        float   contextErase = 0.75f;   // percent of context to erase if we exceed the context
            // window
    };

    explicit LLModel() {}
    virtual ~LLModel() {}

    virtual bool loadModel(const std::string &modelPath) = 0;
    virtual bool isModelLoaded() const = 0;
    virtual size_t stateSize() const { return 0; }
    virtual size_t saveState(uint8_t */*dest*/) const { return 0; }
    virtual size_t restoreState(const uint8_t */*src*/) { return 0; }
    virtual void prompt(const std::string &prompt,
        std::function<bool(int32_t)> promptCallback,
        std::function<bool(int32_t, const std::string&)> responseCallback,
        std::function<bool(bool)> recalculateCallback,
        PromptContext &ctx) = 0;
    virtual void setThreadCount(int32_t /*n_threads*/) {}
    virtual int32_t threadCount() const { return 1; }

    const Implementation& implementation() const {
        return *m_implementation;
    }

    static const std::vector<Implementation>& implementationList();
    static const Implementation *implementation(std::ifstream& f, const std::string& buildVariant);
    static LLModel *construct(const std::string &modelPath, std::string buildVariant = "default");

protected:
    const Implementation *m_implementation = nullptr;

    virtual void recalculateContext(PromptContext &promptCtx,
        std::function<bool(bool)> recalculate) = 0;
};
#endif // LLMODEL_H
