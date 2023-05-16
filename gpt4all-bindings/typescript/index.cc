#include <napi.h>

#include <napi.h>
#include "llmodel.h"

class LLModelWrapper : public Napi::ObjectWrap<LLModelWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "LLModel", {
      InstanceMethod("loadModel", &LLModelWrapper::LoadModel),
      InstanceMethod("isModelLoaded", &LLModelWrapper::IsModelLoaded),
      InstanceMethod("stateSize", &LLModelWrapper::StateSize),
      InstanceMethod("saveState", &LLModelWrapper::SaveState),
      InstanceMethod("restoreState", &LLModelWrapper::RestoreState),
      InstanceMethod("prompt", &LLModelWrapper::Prompt),
      InstanceMethod("setThreadCount", &LLModelWrapper::SetThreadCount),
      InstanceMethod("threadCount", &LLModelWrapper::ThreadCount),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("LLModel", func);
    return exports;
  }

  LLModelWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LLModelWrapper>(info) {
    // Constructor implementation
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
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return LLModelWrapper::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init);
