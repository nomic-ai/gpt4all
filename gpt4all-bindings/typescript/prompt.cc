#include "prompt.h"


TsfnContext::TsfnContext(Napi::Env env, const PromptWorkContext& pc) 
    : deferred_(Napi::Promise::Deferred::New(env)), pc(pc) {
}

std::mutex mtx;
static thread_local std::string res;
bool response_callback(int32_t token_id, const char *response) {
   res+=response;
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
  std::lock_guard<std::mutex> lock(mtx);
  // Perform a call into JavaScript.
  napi_status status =
    context->tsfn.NonBlockingCall(&context->pc,
    [](Napi::Env env, Napi::Function jsCallback, PromptWorkContext* pc) {
        llmodel_prompt(
            *pc->inference_,
            pc->question.c_str(),
            &prompt_callback,
            &response_callback,
            &recalculate_callback,
            &pc->prompt_params
        );
        jsCallback.Call({ Napi::String::New(env, res)} );
        res.clear();
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
  // Join the thread
  context->nativeThread.join();
  // Resolve the Promise previously returned to JS via the CreateTSFN method.
  context->deferred_.Resolve(Napi::Boolean::New(env, true));
  delete context;
}


