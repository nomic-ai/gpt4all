#include <napi.h>
#include <iostream>
#include "llmodel_c.h" 
#include "llmodel.h"
#include "gptj.h"
#include "llamamodel.h"
#include "mpt.h"
#include "stdcapture.h"

class NodeModelWrapper : public Napi::ObjectWrap<NodeModelWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "LLModel", {
      InstanceMethod("type",  &NodeModelWrapper::getType),
      InstanceMethod("name", &NodeModelWrapper::getName),
      InstanceMethod("stateSize", &NodeModelWrapper::StateSize),
      InstanceMethod("raw_prompt", &NodeModelWrapper::Prompt),
      InstanceMethod("setThreadCount", &NodeModelWrapper::SetThreadCount),
      InstanceMethod("threadCount", &NodeModelWrapper::ThreadCount),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("LLModel", func);
    return exports;
  }

  Napi::Value getType(const Napi::CallbackInfo& info) 
  {
    return Napi::String::New(info.Env(), type);
  }

  NodeModelWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NodeModelWrapper>(info) 
  {
    auto env = info.Env();
    std::string weights_path = info[0].As<Napi::String>().Utf8Value();

    const char *c_weights_path = weights_path.c_str();
    
    inference_ = create_model_set_type(c_weights_path);

    auto success = llmodel_loadModel(inference_, c_weights_path);
    if(!success) {
        Napi::Error::New(env, "Failed to load model at given path").ThrowAsJavaScriptException(); 
        return;
    }
    name = weights_path.substr(weights_path.find_last_of("/\\") + 1);
    
  };
  ~NodeModelWrapper() {
    // destroying the model manually causes exit code 3221226505, why?
    // However, bindings seem to operate fine without destructing pointer
    //llmodel_model_destroy(inference_);
  }

  Napi::Value IsModelLoaded(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), llmodel_isModelLoaded(inference_));
  }

  Napi::Value StateSize(const Napi::CallbackInfo& info) {
    // Implement the binding for the stateSize method
    return Napi::Number::New(info.Env(), static_cast<int64_t>(llmodel_get_state_size(inference_)));
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
  Napi::Value Prompt(const Napi::CallbackInfo& info) {

    auto env = info.Env();

    std::string question;
    if(info[0].IsString()) {
        question = info[0].As<Napi::String>().Utf8Value();
    } else {
        Napi::Error::New(env, "invalid string argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    //defaults copied from python bindings
    llmodel_prompt_context promptContext = {
             .logits = nullptr,
             .tokens = nullptr,
             .n_past = 0,
             .n_ctx = 1024,
             .n_predict = 128,
             .top_k = 40,
             .top_p = 0.9f,
             .temp = 0.72f,
             .n_batch = 8,
             .repeat_penalty = 1.0f,
             .repeat_last_n = 10,
             .context_erase = 0.5
         };
    if(info[1].IsObject())
    {
        auto inputObject = info[1].As<Napi::Object>();
             
        // Extract and assign the properties
        if (inputObject.Has("logits") || inputObject.Has("tokens")) {
            Napi::Error::New(env, "Invalid input: 'logits' or 'tokens' properties are not allowed").ThrowAsJavaScriptException();
            return env.Undefined();
        }
             // Assign the remaining properties
             if(inputObject.Has("n_past")) {
                 promptContext.n_past = inputObject.Get("n_past").As<Napi::Number>().Int32Value();
             }
             if(inputObject.Has("n_ctx")) {
                 promptContext.n_ctx = inputObject.Get("n_ctx").As<Napi::Number>().Int32Value();
             }
             if(inputObject.Has("n_predict")) {
                 promptContext.n_predict = inputObject.Get("n_predict").As<Napi::Number>().Int32Value();
             }
             if(inputObject.Has("top_k")) {
                 promptContext.top_k = inputObject.Get("top_k").As<Napi::Number>().Int32Value();
             }
             if(inputObject.Has("top_p")) {
                 promptContext.top_p = inputObject.Get("top_p").As<Napi::Number>().FloatValue();
             }
             if(inputObject.Has("temp")) {
                 promptContext.temp = inputObject.Get("temp").As<Napi::Number>().FloatValue();
             }
             if(inputObject.Has("n_batch")) {
                 promptContext.n_batch = inputObject.Get("n_batch").As<Napi::Number>().Int32Value();
             }
             if(inputObject.Has("repeat_penalty")) {
                 promptContext.repeat_penalty = inputObject.Get("repeat_penalty").As<Napi::Number>().FloatValue();
             }
             if(inputObject.Has("repeat_last_n")) {
                 promptContext.repeat_last_n = inputObject.Get("repeat_last_n").As<Napi::Number>().Int32Value();
             }
             if(inputObject.Has("context_erase")) {
                 promptContext.context_erase = inputObject.Get("context_erase").As<Napi::Number>().FloatValue();
             }
    }
    //    custom callbacks are weird with the gpt4all c bindings: I need to turn Napi::Functions into  raw c function pointers,
    //    but it doesn't seem like its possible? (TODO, is it possible?)

    //    if(info[1].IsFunction()) {
    //        Napi::Callback cb = *info[1].As<Napi::Function>();
    //    }


    // For now, simple capture of stdout
    // possible TODO: put this on a libuv async thread. (AsyncWorker)
    CoutRedirect cr;
    llmodel_prompt(inference_, question.c_str(), &prompt_callback, &response_callback, &recalculate_callback,  &promptContext);
    return Napi::String::New(env, cr.getString());
  }

  void SetThreadCount(const Napi::CallbackInfo& info) {
    if(info[0].IsNumber()) {
        llmodel_setThreadCount(inference_, info[0].As<Napi::Number>().Int64Value());
    } else {
        Napi::Error::New(info.Env(), "Could not set thread count: argument 1 is NaN").ThrowAsJavaScriptException(); 
        return;
    }
  }
  Napi::Value getName(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), name);
  }
  Napi::Value ThreadCount(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), llmodel_threadCount(inference_));
  }

private:
  llmodel_model inference_;
  std::string type;
  std::string name;


  //wrapper cb to capture output into stdout.then, CoutRedirect captures this 
  // and writes it to a file
  static bool response_callback(int32_t tid, const char* resp) 
  {
    if(tid != -1) {
        std::cout<<std::string(resp);
        return true;
    }
    return false;
  }

  static bool prompt_callback(int32_t tid) { return true; }
  static bool recalculate_callback(bool isrecalculating) { return  isrecalculating; }
  // Had to use this instead of the c library in order 
  // set the type of the model loaded.
  // causes side effect: type is mutated;
  llmodel_model create_model_set_type(const char* c_weights_path) 
  {

    uint32_t magic;
    llmodel_model model;
    FILE *f = fopen(c_weights_path, "rb");
    fread(&magic, sizeof(magic), 1, f);

    if (magic == 0x67676d6c) {
        model = llmodel_gptj_create();  
        type = "gptj";
    }
    else if (magic == 0x67676a74) {
        model = llmodel_llama_create(); 
        type = "llama";
    }
    else if (magic == 0x67676d6d) {
        model = llmodel_mpt_create();   
        type = "mpt";
    }
    else  {fprintf(stderr, "Invalid model file\n");}
    fclose(f);
    
    return model;
  }
};

//Exports Bindings
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return NodeModelWrapper::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
