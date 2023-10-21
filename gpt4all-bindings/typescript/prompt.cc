#include "prompt.h"


TsfnContext::TsfnContext(Napi::Env env, const PromptWorkContext& pc) 
    : deferred_(Napi::Promise::Deferred::New(env)), pc(pc) {
}
namespace {
    static std::string *res;
}

bool response_callback(int32_t token_id, const char *response) {
   *res += response;
   return token_id != -1;
}
bool recalculate_callback (bool isrecalculating) {
    return isrecalculating; 
};
bool prompt_callback (int32_t tid) {
    return true; 
};

// The thread entry point. This takes as its arguments the specific
// threadsafe-function context created inside the main thread.
void threadEntry(TsfnContext* context) {
  static std::mutex mtx;
  std::lock_guard<std::mutex> lock(mtx);
  res = &context->pc.res;
  // Perform a call into JavaScript.
  napi_status status =
    context->tsfn.BlockingCall(&context->pc,
    [](Napi::Env env, Napi::Function jsCallback, PromptWorkContext* pc) {
        llmodel_prompt(
            pc->inference_,
            pc->question.c_str(),
            &prompt_callback,
            &response_callback,
            &recalculate_callback,
            &pc->prompt_params
        );
  });

  if (status != napi_ok) {
    Napi::Error::Fatal(
        "ThreadEntry",
        "Napi::ThreadSafeNapi::Function.NonBlockingCall() failed");
  }
  // Release the thread-safe function. This decrements the internal thread
  // count, and will perform finalization since the count will reach 0.
  context->tsfn.Release();
}

void FinalizerCallback(Napi::Env env,
                       void* finalizeData,
                       TsfnContext* context) {
  // Resolve the Promise previously returned to JS 
    context->deferred_.Resolve(Napi::String::New(env, context->pc.res));
    // Wait for the thread to finish executing before proceeding.
    context->nativeThread.join();
    delete context;
}
