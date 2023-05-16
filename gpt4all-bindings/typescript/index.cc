#include <napi.h>

#include "llmodel_c.h" 
#include "llmodel.h"
#include "gptj.h"
#include "llamamodel.h"
#include "mpt.h"


class NodeModelWrapper : public Napi::ObjectWrap<NodeModelWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "LLModel", {
      InstanceMethod("loadModel", &NodeModelWrapper::LoadModel),
      InstanceMethod("isModelLoaded", &NodeModelWrapper::IsModelLoaded),
      InstanceMethod("stateSize", &NodeModelWrapper::StateSize),
      InstanceMethod("saveState", &NodeModelWrapper::SaveState),
      InstanceMethod("restoreState", &NodeModelWrapper::RestoreState),
      InstanceMethod("prompt", &NodeModelWrapper::Prompt),
      InstanceMethod("setThreadCount", &NodeModelWrapper::SetThreadCount),
      InstanceMethod("threadCount", &NodeModelWrapper::ThreadCount),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("LLModel", func);
    return exports;
  }

  NodeModelWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NodeModelWrapper>(info) {
    auto env = info.Env();
    std::string weights_path = info[0].As<Napi::String>().Utf8Value();
    //one of either gptj, mpt, or llama weights (LLModelWrapper);
    const char *c_weights_path = weights_path.c_str();
    llmodel_model model = llmodel_model_create(c_weights_path);
    if(!llmodel_loadModel(model, c_weights_path)) {
        Napi::Error::New(env, "Could not load model").ThrowAsJavaScriptException();
        return;
    }
    inference_ = model;
  };

  Napi::Value LoadModel(const Napi::CallbackInfo& info) {
    // Implement the binding for the loadModel method
    return Napi::Value();
  }

  Napi::Value IsModelLoaded(const Napi::CallbackInfo& info) {
    // Implement the binding for the isModelLoaded method
    return Napi::Value();
  }

  Napi::Value StateSize(const Napi::CallbackInfo& info) {
    // Implement the binding for the stateSize method
    return Napi::Value();
  }

  Napi::Value SaveState(const Napi::CallbackInfo& info) {
    // Implement the binding for the saveState method
    return Napi::Value();
  }

  Napi::Value RestoreState(const Napi::CallbackInfo& info) {
    // Implement the binding for the restoreState method
    return Napi::Value();
  }

  Napi::Value Prompt(const Napi::CallbackInfo& info) {
    // Implement the binding for the prompt method
    return Napi::Value();
  }

  Napi::Value SetThreadCount(const Napi::CallbackInfo& info) {
    // Implement the binding for the setThreadCount method
    return Napi::Value();
  }

  Napi::Value ThreadCount(const Napi::CallbackInfo& info) {
    // Implement the binding for the threadCount method
    return Napi::Value();
  }

private:
  void *inference_;
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return NodeModelWrapper::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
