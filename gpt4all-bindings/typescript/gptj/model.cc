#include <napi.h>

#include "llmodel.h"
#include "gptj.h"

class GPTJWrapper : public Napi::ObjectWrap<GPTJWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "GPTJ", {
      InstanceMethod("loadModel", &GPTJWrapper::LoadModel),
      InstanceMethod("isModelLoaded", &GPTJWrapper::IsModelLoaded),
      InstanceMethod("stateSize", &GPTJWrapper::StateSize),
      InstanceMethod("saveState", &GPTJWrapper::SaveState),
      InstanceMethod("restoreState", &GPTJWrapper::RestoreState),
      InstanceMethod("prompt", &GPTJWrapper::Prompt),
      InstanceMethod("setThreadCount", &GPTJWrapper::SetThreadCount),
      InstanceMethod("threadCount", &GPTJWrapper::ThreadCount),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("GPTJ", func);
    return exports;
  }

  GPTJWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<GPTJWrapper>(info), d_ptr(new GPTJPrivate()) {
    // Constructor implementation
  }

  ~GPTJWrapper() {
    delete d_ptr;
  }

  Napi::Value LoadModel(const Napi::CallbackInfo& info) {
    // Implement the binding for the loadModel method
  }

  Napi::Value IsModelLoaded(const Napi::CallbackInfo& info) {
    // Implement the binding for the isModelLoaded method
  }

  Napi::Value StateSize(const Napi::CallbackInfo& info) {
    // Implement the binding for the stateSize method
  }

  Napi::Value SaveState(const Napi::CallbackInfo& info) {
    // Implement the binding for the saveState method
  }

  Napi::Value RestoreState(const Napi::CallbackInfo& info) {
    // Implement the binding for the restoreState method
  }

  Napi::Value Prompt(const Napi::CallbackInfo& info) {
    // Implement the binding for the prompt method
  }

  Napi::Value SetThreadCount(const Napi::CallbackInfo& info) {
    // Implement the binding for the setThreadCount method
  }

  Napi::Value ThreadCount(const Napi::CallbackInfo& info) {
    // Implement the binding for the threadCount method
  }

private:
  GPTJPrivate* d_ptr;
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return GPTJWrapper::Init(env, exports);
}

