#ifndef PREDICT_WORKER_H
#define PREDICT_WORKER_H

#include "napi.h"
#include "llmodel_c.h"
#include "llmodel.h"
#include <thread>
#include <mutex>
#include <iostream>
#include <atomic>
#include <memory>

struct TokenCallbackInfo
    {
        int32_t tokenId;
        std::string total;
        std::string token;
    };

    struct LLModelWrapper
    {
        LLModel *llModel = nullptr;
        LLModel::PromptContext promptContext;
        ~LLModelWrapper() { delete llModel; }
    };

    struct PromptWorkerConfig
    {
        Napi::Function tokenCallback;
        bool bHasTokenCallback = false;
        llmodel_model model;
        std::mutex * mutex;
        std::string prompt;
        llmodel_prompt_context context;
        std::string result;
    };

    class PromptWorker : public Napi::AsyncWorker
    {
    public:
        PromptWorker(Napi::Env env, PromptWorkerConfig config);
        ~PromptWorker();
        void Execute() override;
        void OnOK() override;
        void OnError(const Napi::Error &e) override;
        Napi::Promise GetPromise();

        bool ResponseCallback(int32_t token_id, const std::string token);
        bool RecalculateCallback(bool isrecalculating);
        bool PromptCallback(int32_t tid);

    private:
        Napi::Promise::Deferred promise;
        std::string result;
        PromptWorkerConfig _config;
        Napi::ThreadSafeFunction _tsfn;
    };

#endif  // PREDICT_WORKER_H
