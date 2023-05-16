#include <napi.h>

#include "llmodel.h"
#include "mpt.h"

class MPTWrapper : public Napi::ObjectWrap<MPTWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "MPT", {
      InstanceMethod("loadModel", &MPTWrapper::LoadModel),
      InstanceMethod("isModelLoaded", &MPTWrapper::IsModelLoaded),
      InstanceMethod("stateSize", &MPTWrapper::StateSize),
      InstanceMethod("saveState", &MPTWrapper::SaveState),
      InstanceMethod("restoreState", &MPTWrapper::RestoreState),
      InstanceMethod("prompt", &MPTWrapper::Prompt),
      InstanceMethod("setThreadCount", &MPTWrapper::SetThreadCount),
      InstanceMethod("threadCount", &MPTWrapper::ThreadCount),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("MPT", func);
    return exports;
  }

  MPTWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<MPTWrapper>(info), d_ptr(new MPTPrivate()) {
    // Constructor implementation
  }

  ~MPTWrapper() {
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
  MPTPrivate* d_ptr;
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return MPTWrapper::Init(env, exports);
}
