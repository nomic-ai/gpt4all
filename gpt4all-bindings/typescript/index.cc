#include <napi.h>
#include <iostream>
#include "llmodel_c.h" 
#include "llmodel.h"
#include "gptj.h"
#include "llamamodel.h"
#include "mpt.h"


class NodeModelWrapper : public Napi::ObjectWrap<NodeModelWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "LLModel", {
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

  static llmodel_response_callback prompt_callback_wrapper(int32_t token_id, const char *response, Napi::Function cb) 
  {
    
  }
  static llmodel_response_callback response_callback_wrapper(int32_t token_id, const char *response, Napi::Function cb) 
  {

  }

  static llmodel_recalculate_callback is_calculating_again_wrapper(bool is_calculating, Napi::Env e, Napi::Function cb) 
  {
  }
  NodeModelWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NodeModelWrapper>(info) {
    auto env = info.Env();
    std::string weights_path = info[0].As<Napi::String>().Utf8Value();
    //one of either gptj, mpt, or llama weights (LLModelWrapper);
    const char *c_weights_path = weights_path.c_str();
    //todo: parse params
    inference_ = llmodel_model_create(c_weights_path);
    auto success = llmodel_loadModel(inference_, c_weights_path);
    if(!success) {
        Napi::Error::New(env, "Failed to load model at given path").ThrowAsJavaScriptException(); 
        return;
    }
  };
  Napi::Value IsModelLoaded(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), llmodel_isModelLoaded(inference_));
  }

  Napi::Value StateSize(const Napi::CallbackInfo& info) {
    // Implement the binding for the stateSize method
    return Napi::Number::New(info.Env(), static_cast<int64_t>(llmodel_get_state_size(inference_)));
  }

  Napi::Value SaveState(const Napi::CallbackInfo& info) {
    // Implement the binding for the saveState method
    return Napi::Value();
  }

  Napi::Value RestoreState(const Napi::CallbackInfo& info) {
    // Implement the binding for the restoreState method
    return Napi::Value();
  }
/**
 * Generate a response using the model.
 * @param model A pointer to the llmodel_model instance.
 * @param prompt A string representing the input prompt.
 * @param prompt_callback A callback function for handling the processing of prompt.
 * @param response_callback A callback function for handling the generated response.
 * @param recalculate_callback A callback function for handling recalculation requests.
 * @param ctx A pointer to the llmodel_prompt_context structure.
 */
  void Prompt(const Napi::CallbackInfo& info) {

    auto env = info.Env();
    std::string question;
    if(info[0].IsString()) {
        question = info[0].As<Napi::String>().Utf8Value();
    } else {
        Napi::Error::New(env, "invalid string argument").ThrowAsJavaScriptException();
        return;
    }

    llmodel_response_callback prompt_callback;
    llmodel_response_callback response_callback;
    llmodel_recalculate_callback recalculate_callback;

    if(info[1].IsFunction()) {
        prompt_callback = info[1].As<Napi::Function>();

    } else {
       Napi::Error::New(env, "invalid string argument").ThrowAsJavaScriptException();
       return;
    }
    if(info[2].IsFunction()) {
        response_callback = info[2].As<Napi::Function>();
        
    }
    if(info[3].IsFunction()) {
        recalculate_callback = info[3].As<Napi::Function>();
    }
    llmodel_prompt(inference_, question, prompt_callback, response_callback, recalculate_callback);  
  }

  void SetThreadCount(const Napi::CallbackInfo& info) {
    if(info[0].IsNumber()) {
        llmodel_setThreadCount(inference_, info[0].As<Napi::Number>().Int64Value());
    } else {
        Napi::Error::New(info.Env(), "Could not set thread count: argument 1 is NaN").ThrowAsJavaScriptException(); 
        return;
    }
  }

  Napi::Value ThreadCount(const Napi::CallbackInfo& info) {
    // Implement the binding for the threadCount method
    return Napi::Number::New(info.Env(), llmodel_threadCount(inference_));
  }

private:
  llmodel_model inference_;
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return NodeModelWrapper::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
