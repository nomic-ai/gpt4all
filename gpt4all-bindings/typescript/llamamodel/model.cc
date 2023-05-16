#include <napi.h>

#include "llmodel.h"
#include "llamamodel.h"


#include <napi.h>
#include "LLamaModel.h"

class LLamaModelWrapper : public Napi::ObjectWrap<LLamaModelWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "LLamaModel", {
      InstanceMethod("loadModel", &LLamaModelWrapper::LoadModel),
      InstanceMethod("isModelLoaded", &LLamaModelWrapper::IsModelLoaded),
      InstanceMethod("stateSize", &LLamaModelWrapper::StateSize),
      InstanceMethod("saveState", &LLamaModelWrapper::SaveState),
      InstanceMethod("restoreState", &LLamaModelWrapper::RestoreState),
      InstanceMethod("prompt", &LLamaModelWrapper::Prompt),
      InstanceMethod("setThreadCount", &LLamaModelWrapper::SetThreadCount),
      InstanceMethod("threadCount", &LLamaModelWrapper::ThreadCount),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("LLamaModel", func);
    return exports;
  }

  LLamaModelWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LLamaModelWrapper>(info), d_ptr(new LLamaPrivate()) {
    // Constructor implementation
  }

  ~LLamaModelWrapper() {
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
  LLamaPrivate* d_ptr;
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return LLamaModelWrapper::Init(env, exports);
}

