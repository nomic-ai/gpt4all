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
      InstanceMethod("type",  &NodeModelWrapper::getType),
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

  Napi::Value getType(const Napi::CallbackInfo& info) 
  {
    return Napi::String::New(info.Env(), type);
  }
  NodeModelWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NodeModelWrapper>(info) {
    auto env = info.Env();
    std::string weights_path = info[0].As<Napi::String>().Utf8Value();

    const char *c_weights_path = weights_path.c_str();
    
    inference_ = create_model_set_type(c_weights_path);

    auto success = llmodel_loadModel(inference_, c_weights_path);
    if(!success) {
        Napi::Error::New(env, "Failed to load model at given path").ThrowAsJavaScriptException(); 
        return;
    }
    
  };
  ~NodeModelWrapper() {
    //this causes exit code 3221226505, why?
    // without destroying the model manually, everything is okay so far.
    //llmodel_model_destroy(inference_);
  }
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
    /**
        * llmodel_prompt_context structure for holding the prompt context.
        * NOTE: The implementation takes care of all the memory handling of the raw logits pointer and the
        * raw tokens pointer. Attempting to resize them or modify them in any way can lead to undefined
        * behavior.

     typedef struct {
         float *logits;          // logits of current context
         size_t logits_size;     // the size of the raw logits vector
         int32_t *tokens;        // current tokens in the context window
         size_t tokens_size;     // the size of the raw tokens vector
         int32_t n_past;         // number of tokens in past conversation
         int32_t n_ctx;          // number of tokens possible in context window
         int32_t n_predict;      // number of tokens to predict
         int32_t top_k;          // top k logits to sample from
         float top_p;            // nucleus sampling probability threshold
         float temp;             // temperature to adjust model's output distribution
         int32_t n_batch;        // number of predictions to generate in parallel
         float repeat_penalty;   // penalty factor for repeated tokens
         int32_t repeat_last_n;  // last n tokens to penalize
         float context_erase;    // percent of context to erase if we exceed the context window
     } llmodel_prompt_context;
     */
     llmodel_prompt_context promptContext = {
         .logits = nullptr,
         .logits_size = 0,
         .tokens = nullptr,
         .tokens_size = 0,
         .n_past = 0,
         .n_ctx = 1024,
         .n_predict = 50,
         .top_k = 10,
         .top_p = 0.9,
         .temp = 1.0,
         .n_batch = 1,
         .repeat_penalty = 1.2,
         .repeat_last_n = 10,
         .context_erase = 0.5
     };
    if(info[1].IsObject())
    {
        auto inputObject = info[1].As<Napi::Object>();
        
        // Extract and assign the properties
        if (inputObject.Has("logits") || inputObject.Has("tokens")) {
            Napi::Error::New(env, "Invalid input: 'logits' or 'tokens' properties are not allowed").ThrowAsJavaScriptException();
            return;
        }
        // Assign the remaining properties
        if(inputObject.Has("n_past")) 
        {
            promptContext.n_past = inputObject.Get("n_past").As<Napi::Number>().Int32Value();
        }
        if(inputObject.Has("n_ctx")) 
        {
            promptContext.n_ctx = inputObject.Get("n_ctx").As<Napi::Number>().Int32Value();
        }
        if(inputObject.Has("n_predict")) 
        {
            promptContext.n_predict = inputObject.Get("n_predict").As<Napi::Number>().Int32Value();
        }
        if(inputObject.Has("top_k")) 
        {
            promptContext.top_k = inputObject.Get("top_k").As<Napi::Number>().Int32Value();
        }
        if(inputObject.Has("top_p")) 
        {
            promptContext.top_p = inputObject.Get("top_p").As<Napi::Number>().FloatValue();
        }
        if(inputObject.Has("temp")) 
        {
            promptContext.temp = inputObject.Get("temp").As<Napi::Number>().FloatValue();
        }
        if(inputObject.Has("n_batch")) 
        {
            promptContext.n_batch = inputObject.Get("n_batch").As<Napi::Number>().Int32Value();
        }
        if(inputObject.Has("repeat_penalty"))
        {
            promptContext.repeat_penalty = inputObject.Get("repeat_penalty").As<Napi::Number>().FloatValue();
        }
        if(inputObject.Has("repeat_last_n"))
        {
            promptContext.repeat_last_n = inputObject.Get("repeat_last_n").As<Napi::Number>().Int32Value();
        }
        if(inputObject.Has("context_erase"))
        {
            promptContext.context_erase = inputObject.Get("context_erase").As<Napi::Number>().FloatValue();
        }
    
    }

    llmodel_prompt(inference_, question.c_str(), &prompt_callback, &response_callback, &recalculate_callback, &promptContext);  

//    custom callbacks are weird with the gpt4all c bindings: I need to turn Napi::Functions into  raw c function pointers,
//    but it doesn't seem like its possible? (TODO, is it possible?)

//    if(info[1].IsFunction()) {
//        Napi::Callback cb = *info[1].As<Napi::Function>();
//    }
//    if(info[2].IsFunction()) {
//        Napi::Callback cb = *info[2].As<Napi::Function>(); 
//    }
//    if(info[3].IsFunction()) {
//        Napi::Callback cb = *info[3].As<Napi::Function>(); 
//    }
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
    return Napi::Number::New(info.Env(), llmodel_threadCount(inference_));
  }

private:
  llmodel_model inference_;
  std::string type;
  static bool response_callback(int32_t tid, const char* resp) 
  {
    return true;
  }

  static bool prompt_callback(int32_t tid, const char* resp)
  {
    return true;
  }
    
  static bool recalculate_callback(bool isrecalculating)
  {
    return isrecalculating;
  }
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

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return NodeModelWrapper::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
