#include "prompt.h"
#include <future>

PromptWorker::PromptWorker(Napi::Env env, PromptWorkerConfig config)
        : promise(Napi::Promise::Deferred::New(env)), _config(config), AsyncWorker(env) {
            if(_config.bHasTokenCallback){
                _tsfn = Napi::ThreadSafeFunction::New(config.tokenCallback.Env(),config.tokenCallback,"PromptWorker",0,1,this);
            }
        }

    PromptWorker::~PromptWorker()
    {
        if(_config.bHasTokenCallback){
            _tsfn.Release();
        }
    }

    void PromptWorker::Execute()
    {
        _config.mutex->lock();

        LLModelWrapper *wrapper = reinterpret_cast<LLModelWrapper *>(_config.model);

        auto ctx = &_config.context;

        if (size_t(ctx->n_past) < wrapper->promptContext.tokens.size())
            wrapper->promptContext.tokens.resize(ctx->n_past);

        // Copy the C prompt context
        wrapper->promptContext.n_past = ctx->n_past;
        wrapper->promptContext.n_ctx = ctx->n_ctx;
        wrapper->promptContext.n_predict = ctx->n_predict;
        wrapper->promptContext.top_k = ctx->top_k;
        wrapper->promptContext.top_p = ctx->top_p;
        wrapper->promptContext.temp = ctx->temp;
        wrapper->promptContext.n_batch = ctx->n_batch;
        wrapper->promptContext.repeat_penalty = ctx->repeat_penalty;
        wrapper->promptContext.repeat_last_n = ctx->repeat_last_n;
        wrapper->promptContext.contextErase = ctx->context_erase;

        // Napi::Error::Fatal(
        //         "SUPRA",
        //         "About to prompt");
        // Call the C++ prompt method
        wrapper->llModel->prompt(
            _config.prompt, 
            [](int32_t tid) { return true; },
            [this](int32_t token_id, const std::string tok)
            {
                return ResponseCallback(token_id, tok);
            },
            [](bool isRecalculating)
            {
                return isRecalculating;
            },
            wrapper->promptContext);

        // Update the C context by giving access to the wrappers raw pointers to std::vector data
        // which involves no copies
        ctx->logits = wrapper->promptContext.logits.data();
        ctx->logits_size = wrapper->promptContext.logits.size();
        ctx->tokens = wrapper->promptContext.tokens.data();
        ctx->tokens_size = wrapper->promptContext.tokens.size();

        // Update the rest of the C prompt context
        ctx->n_past = wrapper->promptContext.n_past;
        ctx->n_ctx = wrapper->promptContext.n_ctx;
        ctx->n_predict = wrapper->promptContext.n_predict;
        ctx->top_k = wrapper->promptContext.top_k;
        ctx->top_p = wrapper->promptContext.top_p;
        ctx->temp = wrapper->promptContext.temp;
        ctx->n_batch = wrapper->promptContext.n_batch;
        ctx->repeat_penalty = wrapper->promptContext.repeat_penalty;
        ctx->repeat_last_n = wrapper->promptContext.repeat_last_n;
        ctx->context_erase = wrapper->promptContext.contextErase;

        _config.mutex->unlock();
    }

    void PromptWorker::OnOK()
    {
        promise.Resolve(Napi::String::New(Env(), result));
    }

    void PromptWorker::OnError(const Napi::Error &e)
    {
        promise.Reject(e.Value());
    }

    Napi::Promise PromptWorker::GetPromise()
    {
        return promise.Promise();
    }

    bool PromptWorker::ResponseCallback(int32_t token_id, const std::string token)
    {
        if (token_id == -1)
        {
            return false;
        }

        if(!_config.bHasTokenCallback){
            return true;
        }

        result += token;

       std::promise<bool> promise;

       auto info = new TokenCallbackInfo();
       info->tokenId = token_id;
       info->token = token;
       info->total = result;

       auto future = promise.get_future();

       auto status = _tsfn.BlockingCall(info, [&promise](Napi::Env env, Napi::Function jsCallback, TokenCallbackInfo *value)
       {
           // Transform native data into JS data, passing it to the provided
           // `jsCallback` -- the TSFN's JavaScript function.
           auto token_id = Napi::Number::New(env, value->tokenId);
           auto token = Napi::String::New(env, value->token);
           auto total = Napi::String::New(env,value->total);
           auto jsResult = jsCallback.Call({ token_id, token, total}).ToBoolean();
           promise.set_value(jsResult);
           // We're finished with the data.
           delete value;
       });
       if (status != napi_ok) {
            Napi::Error::Fatal(
                "PromptWorkerResponseCallback",
                "Napi::ThreadSafeNapi::Function.NonBlockingCall() failed");
        }

       return future.get();
    }

    bool PromptWorker::RecalculateCallback(bool isRecalculating)
    {
        return isRecalculating;
    }

    bool PromptWorker::PromptCallback(int32_t tid)
    {
        return true;
    }
