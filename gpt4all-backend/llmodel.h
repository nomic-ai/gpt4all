#ifndef LLMODEL_H
#define LLMODEL_H

#include <string>
#include <functional>
#include <vector>
#include <string_view>
#include <fstream>
#include <cstdint>
#include <limits>

#define LLMODEL_MAX_PROMPT_BATCH 128

class Dlhandle;
class LLModel {
public:
    using Token = int32_t;
    class Implementation {
    public:
        Implementation(Dlhandle&&);
        Implementation(const Implementation&) = delete;
        Implementation(Implementation&&);
        ~Implementation();

        std::string_view modelType() const { return m_modelType; }
        std::string_view buildVariant() const { return m_buildVariant; }

        static bool isImplementation(const Dlhandle&);
        static const std::vector<Implementation>& implementationList();
        static const Implementation *implementation(std::ifstream& f, const std::string& buildVariant);
        static LLModel *construct(const std::string &modelPath, std::string buildVariant = "auto");
        static void setImplementationsSearchPath(const std::string& path);
        static const std::string& implementationsSearchPath();

    private:
        bool (*m_magicMatch)(std::ifstream& f);
        LLModel *(*m_construct)();

    private:
        std::string_view m_modelType;
        std::string_view m_buildVariant;
        Dlhandle *m_dlhandle;
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

    virtual bool supportsEmbedding() const = 0;
    virtual bool supportsCompletion() const = 0;
    virtual bool loadModel(const std::string &modelPath) = 0;
    virtual bool isModelLoaded() const = 0;
    virtual size_t requiredMem(const std::string &modelPath) = 0;
    virtual size_t stateSize() const { return 0; }
    virtual size_t saveState(uint8_t */*dest*/) const { return 0; }
    virtual size_t restoreState(const uint8_t */*src*/) { return 0; }

    // This method requires the model to return true from supportsCompletion otherwise it will throw
    // an error
    virtual void prompt(const std::string &prompt,
                        std::function<bool(int32_t)> promptCallback,
                        std::function<bool(int32_t, const std::string&)> responseCallback,
                        std::function<bool(bool)> recalculateCallback,
                        PromptContext &ctx);

    virtual std::vector<float> embedding(const std::string &text);

    virtual void setThreadCount(int32_t /*n_threads*/) {}
    virtual int32_t threadCount() const { return 1; }

    const Implementation& implementation() const {
        return *m_implementation;
    }

protected:
    // These are pure virtual because subclasses need to implement as the default implementation of
    // 'prompt' above calls these functions
    virtual std::vector<Token> tokenize(PromptContext &, const std::string&) const = 0;
    virtual std::string tokenToString(Token) const = 0;
    virtual Token sampleToken(PromptContext &ctx) const = 0;
    virtual bool evalTokens(PromptContext &/*ctx*/, const std::vector<int32_t>& /*tokens*/) const = 0;
    virtual int32_t contextLength() const = 0;
    virtual const std::vector<Token>& endTokens() const = 0;

    // This is a helper function called from the default implementation of 'prompt' but it can be
    // shared by all base classes so it isn't virtual
    void recalculateContext(PromptContext &promptCtx, std::function<bool(bool)> recalculate);

    const Implementation *m_implementation = nullptr;

private:
    friend class LLMImplementation;
};

#endif // LLMODEL_H
