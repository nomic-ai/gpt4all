#ifndef PREDICT_WORKER_H
#define PREDICT_WORKER_H

#include "llmodel.h"
#include "llmodel_c.h"
#include "napi.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

struct ResponseCallbackData
{
    int32_t tokenId;
    std::string token;
};

struct PromptCallbackData
{
    int32_t tokenId;
};

struct LLModelWrapper
{
    LLModel *llModel = nullptr;
    LLModel::PromptContext promptContext;
    ~LLModelWrapper()
    {
        delete llModel;
    }
};

struct PromptWorkerConfig
{
    Napi::Function responseCallback;
    bool hasResponseCallback = false;
    Napi::Function promptCallback;
    bool hasPromptCallback = false;
    llmodel_model model;
    std::mutex *mutex;
    std::string prompt;
    std::string promptTemplate;
    llmodel_prompt_context context;
    std::string result;
    bool special = false;
    std::string *fakeReply = nullptr;
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
    bool PromptCallback(int32_t token_id);

  private:
    Napi::Promise::Deferred promise;
    std::string result;
    PromptWorkerConfig _config;
    Napi::ThreadSafeFunction _responseCallbackFn;
    Napi::ThreadSafeFunction _promptCallbackFn;
};

#endif // PREDICT_WORKER_H
