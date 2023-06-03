#ifndef TSFN_CONTEXT_H
#define TSFN_CONTEXT_H

#include "napi.h"
#include "llmodel_c.h"
#include <thread>
#include <mutex>
#include <iostream>

struct PromptWorkContext {
    const char* question;
    llmodel_model inference_;
    llmodel_prompt_context prompt_params;
};

struct TsfnContext {
public:
  TsfnContext(Napi::Env env, PromptWorkContext &pc);
  std::thread nativeThread;
  std::mutex mtx;
  PromptWorkContext pc;
  Napi::ThreadSafeFunction tsfn;
  Napi::Promise GetPromise() const;
  Napi::Promise::Deferred GetDeferred() const;
  // You can add more member functions here as needed

private:
  // Native Promise returned to JavaScript
  Napi::Promise::Deferred deferred_;

  // Native thread
  // std::thread nativeThread;

  // Some data to pass around
  // int ints[ARRAY_LENGTH];

};

// The thread entry point. This takes as its arguments the specific
// threadsafe-function context created inside the main thread.
void threadEntry(TsfnContext* context);

// The thread-safe function finalizer callback. This callback executes
// at destruction of thread-safe function, taking as arguments the finalizer
// data and threadsafe-function context.
void FinalizerCallback(Napi::Env env, void* finalizeData, TsfnContext* context);


#endif  // TSFN_CONTEXT_H
