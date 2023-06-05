#include "prompt.h"


TsfnContext::TsfnContext(Napi::Env env, PromptWorkContext& pc) 
    : deferred_(Napi::Promise::Deferred::New(env)), pc(pc) {
}

Napi::Promise TsfnContext::GetPromise() const {
    return deferred_.Promise();
}

Napi::Promise::Deferred TsfnContext::GetDeferred() const {
    return deferred_;
}


thread_local std::string res;
// The thread entry point. This takes as its arguments the specific
// threadsafe-function context created inside the main thread.
void threadEntry(TsfnContext* context) {
  //context->mtx.lock();
  // This callback transforms the native addon data (int *data) to JavaScript
  // values. It also receives the treadsafe-function's registered callback, and
  // may choose to call it.

  auto callback = [](Napi::Env env, Napi::Function jsCallback, PromptWorkContext* pc) {
    auto recalculate_callback = [](bool isrecalculating) {  return isrecalculating; };
    auto prompt_callback = [](int32_t tid) { return true; };
    auto response_callback = [](int32_t tid, const char* resp) {
        res+=resp;
        return tid != -1;
    };
    llmodel_prompt(pc->inference_, pc->question, prompt_callback, response_callback, recalculate_callback, &pc->prompt_params);
    jsCallback.Call({ Napi::String::New(env, res)});
    res = "";
  };
  std::cout << (context->pc.inference_ == nullptr);
  
  // Perform a call into JavaScript.
  napi_status status =
      context->tsfn.NonBlockingCall(&(context->pc), callback);
  if (status != napi_ok) {
    Napi::Error::Fatal(
        "ThreadEntry",
        "Napi::ThreadSafeNapi::Function.NonBlockingCall() failed");
  }
  //context->mtx.unlock();
  // Release the thread-safe function. This decrements the internal thread
  // count, and will perform finalization since the count will reach 0.
  context->tsfn.Release();
}

void FinalizerCallback(Napi::Env env,
                       void* finalizeData,
                       TsfnContext* context) {
  // Join the thread
  context->nativeThread.join();
  std::cout << "finalized";
  // Resolve the Promise previously returned to JS via the CreateTSFN method.
  context->GetDeferred().Resolve(Napi::Boolean::New(env, true));
  delete context;
}

