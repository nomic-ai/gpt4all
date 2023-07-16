#ifndef TSFN_CONTEXT_H
#define TSFN_CONTEXT_H

#include "napi.h"
#include "llmodel_c.h"
#include <thread>
#include <mutex>
#include <iostream>
#include <atomic>
#include <memory>
struct PromptWorkContext {
    std::string question;
    std::shared_ptr<llmodel_model>& inference_;
    llmodel_prompt_context prompt_params;
    std::string res;

};

struct TsfnContext {
public:
  TsfnContext(Napi::Env env, const PromptWorkContext &pc);
  std::thread nativeThread;
  Napi::Promise::Deferred deferred_;
  PromptWorkContext pc;
  Napi::ThreadSafeFunction tsfn;

  // Some data to pass around
  // int ints[ARRAY_LENGTH];

};

// The thread entry point. This takes as its arguments the specific
// threadsafe-function context created inside the main thread.
void threadEntry(TsfnContext*);

// The thread-safe function finalizer callback. This callback executes
// at destruction of thread-safe function, taking as arguments the finalizer
// data and threadsafe-function context.
void FinalizerCallback(Napi::Env, void* finalizeData, TsfnContext*);

bool response_callback(int32_t token_id, const char *response);
bool recalculate_callback (bool isrecalculating);
bool prompt_callback (int32_t tid); 
#endif  // TSFN_CONTEXT_H
