#ifndef LLMODEL_H
#define LLMODEL_H
#include "dlhandle.h"

#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <string_view>
#include <fstream>
#include <cstdint>

class LLModel {
public:
    class Implementation {
        LLModel *(*construct_)();

    public:
        // FIXME: Move the whole implementation details to cpp file
        Implementation(Dlhandle&&);

        static bool isImplementation(const Dlhandle&);

        std::string_view modelType, buildVariant;
        bool (*magicMatch)(std::ifstream& f);
        Dlhandle dlhandle;

        LLModel *construct() const {
            auto fres = construct_();
            fres->implementation = this;
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

    struct PromptCallbacks {
        friend class LLModel;

        // Executed whenever given token has been evaluated
        std::function<void(int32_t)> promptCallback = [] (int32_t) {};
        // Executed whenever given token has been generated (string is token as string)
        std::function<void(int32_t, const std::string&)> responseCallback = [] (int32_t, const std::string&) {};
        // Executed given `true` whenever context recalculation is running, `false` on completion
        std::function<void(bool)> recalculateCallback = [] (bool) {};
        // Executed whenever progress has been made in evaluating tokens, given float is percentage
        std::function<void(float)> promptProgressCallback = [] (float progress) {
            std::cout << "\r" << unsigned(progress) << "% " << std::flush;
        };
        // Should not be touched outside a model implementation; will be reset by prompt()
        bool should_stop = false;

        // To be called whenever you want prompt() to stop asap
        void stop() {
            should_stop = true;
        }
    };

    explicit LLModel() {}
    virtual ~LLModel() {}

    virtual bool loadModel(const std::string &modelPath) = 0;
    virtual bool isModelLoaded() const = 0;
    virtual size_t stateSize() const { return 0; }
    virtual size_t saveState(uint8_t */*dest*/) const { return 0; }
    virtual size_t restoreState(const uint8_t */*src*/) { return 0; }

    // FIXME: This is unused??
    const Implementation& getImplementation() const {
        return *implementation;
    }

    virtual void prompt(const std::string &prompt, PromptCallbacks&, PromptContext &) = 0;
    virtual void setThreadCount(int32_t /*n_threads*/) {}
    virtual int32_t threadCount() const { return 1; }

    [[deprecated("Use `prompt(const std::string &, PromptCallbacks&, PromptContext &)` instead.")]]
    void prompt(const std::string &prompt_,
                std::function<bool(int32_t)> promptCallback,
                std::function<bool(int32_t, const std::string&)> responseCallback,
                std::function<bool(bool)> recalculateCallback,
                PromptContext &ctx);

    // FIXME: Maybe have an 'ImplementationInfo' class for the GUI here, but the DLHandle stuff should
    // be hidden in cpp file
    // FIXME: Avoid usage of 'get' for getters
    static const std::vector<Implementation>& getImplementationList();
    static const Implementation *getImplementation(std::ifstream& f, const std::string& buildVariant);
    static LLModel *construct(const std::string &modelPath, std::string buildVariant = "default");

protected:
    const Implementation *implementation; // FIXME: This is dangling! You don't initialize it in ctor either

    virtual void recalculateContext(PromptContext &promptCtx, PromptCallbacks&) = 0;

    // Would like to have this function directly in class PromptCallbacks, but I'd be forced to make them public...
    void reportPromptProgress(PromptCallbacks& cbs, std::size_t index, std::size_t size) {
        cbs.promptProgressCallback(float(index) / size * 100.f);
    }
    void reportPromptBeginning(PromptCallbacks& cbs) {
        cbs.promptProgressCallback(0.f);
    }
    void reportPromptCompletion(PromptCallbacks& cbs) {
        cbs.promptProgressCallback(100.f);
    }

    const char *modelType;
};
#endif // LLMODEL_H
