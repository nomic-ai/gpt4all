#include "index.h"


Napi::Function NodeModelWrapper::GetClass(Napi::Env env) {
    Napi::Function self = DefineClass(env, "LLModel", {
       InstanceMethod("type",  &NodeModelWrapper::getType),
       InstanceMethod("isModelLoaded", &NodeModelWrapper::IsModelLoaded),
       InstanceMethod("name", &NodeModelWrapper::getName),
       InstanceMethod("stateSize", &NodeModelWrapper::StateSize),
       InstanceMethod("raw_prompt", &NodeModelWrapper::Prompt),
       InstanceMethod("setThreadCount", &NodeModelWrapper::SetThreadCount),
       InstanceMethod("embed", &NodeModelWrapper::GenerateEmbedding),
       InstanceMethod("threadCount", &NodeModelWrapper::ThreadCount),
       InstanceMethod("getLibraryPath", &NodeModelWrapper::GetLibraryPath),
       InstanceMethod("initGpuByString", &NodeModelWrapper::InitGpuByString),
       InstanceMethod("hasGpuDevice", &NodeModelWrapper::HasGpuDevice),
       InstanceMethod("listGpu", &NodeModelWrapper::GetGpuDevices),
       InstanceMethod("memoryNeeded", &NodeModelWrapper::GetRequiredMemory),
       InstanceMethod("dispose", &NodeModelWrapper::Dispose)
    });
    // Keep a static reference to the constructor
    //
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(self);
    env.SetInstanceData(constructor);
    return self;
}
Napi::Value NodeModelWrapper::GetRequiredMemory(const Napi::CallbackInfo& info) 
{
    auto env = info.Env();
    return Napi::Number::New(env, static_cast<uint32_t>( llmodel_required_mem(GetInference(), full_model_path.c_str(), 2048, 100) ));

}
  Napi::Value NodeModelWrapper::GetGpuDevices(const Napi::CallbackInfo& info) 
  {
    auto env = info.Env();
    int num_devices = 0;
    auto mem_size = llmodel_required_mem(GetInference(), full_model_path.c_str());
    llmodel_gpu_device* all_devices = llmodel_available_gpu_devices(GetInference(), mem_size, &num_devices);
    if(all_devices == nullptr) {
        Napi::Error::New(
            env, 
            "Unable to retrieve list of all GPU devices"
        ).ThrowAsJavaScriptException(); 
        return env.Undefined();
    }
    auto js_array = Napi::Array::New(env, num_devices);
    for(int i = 0; i < num_devices; ++i) {
       auto gpu_device = all_devices[i];
       /* 
        *
        * struct llmodel_gpu_device {
            int index = 0;
            int type = 0;           // same as VkPhysicalDeviceType
            size_t heapSize = 0; 
            const char * name;
            const char * vendor;
          };
        *
        */
       Napi::Object js_gpu_device = Napi::Object::New(env);
        js_gpu_device["index"] = uint32_t(gpu_device.index);
        js_gpu_device["type"] = uint32_t(gpu_device.type);
        js_gpu_device["heapSize"] = static_cast<uint32_t>( gpu_device.heapSize );
        js_gpu_device["name"]= gpu_device.name;
        js_gpu_device["vendor"] = gpu_device.vendor;

        js_array[i] = js_gpu_device;
    }
    return js_array;
  }

  Napi::Value NodeModelWrapper::getType(const Napi::CallbackInfo& info) 
  {
    if(type.empty()) {
        return info.Env().Undefined();
    } 
    return Napi::String::New(info.Env(), type);
  }

  Napi::Value NodeModelWrapper::InitGpuByString(const Napi::CallbackInfo& info) 
  {
    auto env = info.Env();
    size_t memory_required = static_cast<size_t>(info[0].As<Napi::Number>().Uint32Value());
    
    std::string gpu_device_identifier = info[1].As<Napi::String>();   

    size_t converted_value;
    if(memory_required <= std::numeric_limits<size_t>::max()) {
        converted_value = static_cast<size_t>(memory_required);
    } else {
         Napi::Error::New(
            env, 
            "invalid number for memory size. Exceeded bounds for memory."
        ).ThrowAsJavaScriptException(); 
        return env.Undefined();
    }
    
    auto result = llmodel_gpu_init_gpu_device_by_string(GetInference(), converted_value, gpu_device_identifier.c_str());
    return Napi::Boolean::New(env, result);
  }
  Napi::Value NodeModelWrapper::HasGpuDevice(const Napi::CallbackInfo& info) 
  {
    return Napi::Boolean::New(info.Env(), llmodel_has_gpu_device(GetInference()));
  }

  NodeModelWrapper::NodeModelWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<NodeModelWrapper>(info) 
  {
    auto env = info.Env();
    fs::path model_path;

    std::string full_weight_path,
                library_path = ".",
                model_name, 
                device;
    if(info[0].IsString()) {
        model_path = info[0].As<Napi::String>().Utf8Value();
        full_weight_path = model_path.string();
        std::cout << "DEPRECATION: constructor accepts object now. Check docs for more.\n";
    } else {
        auto config_object = info[0].As<Napi::Object>();
        model_name = config_object.Get("model_name").As<Napi::String>();
        model_path = config_object.Get("model_path").As<Napi::String>().Utf8Value(); 
        if(config_object.Has("model_type")) {
            type = config_object.Get("model_type").As<Napi::String>(); 
        }
        full_weight_path = (model_path / fs::path(model_name)).string();
        
        if(config_object.Has("library_path")) {
            library_path = config_object.Get("library_path").As<Napi::String>(); 
        } else {
            library_path = ".";
        }
        device = config_object.Get("device").As<Napi::String>();
    }
    llmodel_set_implementation_search_path(library_path.c_str());
    const char* e;
    inference_ = llmodel_model_create2(full_weight_path.c_str(), "auto", &e);
    if(!inference_) {
       Napi::Error::New(env, e).ThrowAsJavaScriptException();
       return;
    }
    if(GetInference() == nullptr) {
       std::cerr << "Tried searching libraries in \"" << library_path << "\"" <<  std::endl;
       std::cerr << "Tried searching for model weight in \"" << full_weight_path << "\"" << std::endl;
       std::cerr << "Do you have runtime libraries installed?" << std::endl;
       Napi::Error::New(env, "Had an issue creating llmodel object, inference is null").ThrowAsJavaScriptException(); 
       return;
    }
    if(device != "cpu") {
        size_t mem = llmodel_required_mem(GetInference(), full_weight_path.c_str());
        std::cout << "Initiating GPU\n";

        auto success = llmodel_gpu_init_gpu_device_by_string(GetInference(), mem, device.c_str());
        if(success) {
            std::cout << "GPU init successfully\n";
        } else {
            //https://github.com/nomic-ai/gpt4all/blob/3acbef14b7c2436fe033cae9036e695d77461a16/gpt4all-bindings/python/gpt4all/pyllmodel.py#L215
            //Haven't implemented this but it is still open to contribution
            std::cout << "WARNING: Failed to init GPU\n";
        }
    }

    auto success = llmodel_loadModel(GetInference(), full_weight_path.c_str(), 2048, 100);
    if(!success) {
        Napi::Error::New(env, "Failed to load model at given path").ThrowAsJavaScriptException(); 
        return;
    }

    name = model_name.empty() ? model_path.filename().string() : model_name;
    full_model_path = full_weight_path;
  };

//  NodeModelWrapper::~NodeModelWrapper() {
//    if(GetInference() != nullptr) {
//        std::cout << "Debug: deleting model\n";
//        llmodel_model_destroy(inference_);
//        std::cout << (inference_ == nullptr);
//    }
//  }
//  void NodeModelWrapper::Finalize(Napi::Env env) {
//    if(inference_ != nullptr) {
//        std::cout << "Debug: deleting model\n";
//
//    } 
//  }
  Napi::Value NodeModelWrapper::IsModelLoaded(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), llmodel_isModelLoaded(GetInference()));
  }

  Napi::Value NodeModelWrapper::StateSize(const Napi::CallbackInfo& info) {
    // Implement the binding for the stateSize method
    return Napi::Number::New(info.Env(), static_cast<int64_t>(llmodel_get_state_size(GetInference())));
  }
  
  Napi::Value NodeModelWrapper::GenerateEmbedding(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    std::string text = info[0].As<Napi::String>().Utf8Value();
    size_t embedding_size = 0;
    float* arr = llmodel_embedding(GetInference(), text.c_str(), &embedding_size);
    if(arr == nullptr) {
        Napi::Error::New(
            env, 
            "Cannot embed. native embedder returned 'nullptr'"
        ).ThrowAsJavaScriptException(); 
        return env.Undefined();
    }

    if(embedding_size == 0 && text.size() != 0 ) {
        std::cout << "Warning: embedding length 0 but input text length > 0" << std::endl;
    }
    Napi::Float32Array js_array = Napi::Float32Array::New(env, embedding_size);
    
    for (size_t i = 0; i < embedding_size; ++i) {
        float element = *(arr + i);
        js_array[i] = element;
    }

    llmodel_free_embedding(arr);

    return js_array;
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
  Napi::Value NodeModelWrapper::Prompt(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    std::string question;
    if(info[0].IsString()) {
        question = info[0].As<Napi::String>().Utf8Value();
    } else {
        Napi::Error::New(info.Env(), "invalid string argument").ThrowAsJavaScriptException();
        return info.Env().Undefined();
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
           Napi::Error::New(info.Env(), "Invalid input: 'logits' or 'tokens' properties are not allowed").ThrowAsJavaScriptException();
           return info.Env().Undefined();
       }
             // Assign the remaining properties
       if(inputObject.Has("n_past")) 
            promptContext.n_past = inputObject.Get("n_past").As<Napi::Number>().Int32Value();
       if(inputObject.Has("n_ctx")) 
            promptContext.n_ctx = inputObject.Get("n_ctx").As<Napi::Number>().Int32Value();
       if(inputObject.Has("n_predict"))
            promptContext.n_predict = inputObject.Get("n_predict").As<Napi::Number>().Int32Value();
       if(inputObject.Has("top_k"))
            promptContext.top_k = inputObject.Get("top_k").As<Napi::Number>().Int32Value();
       if(inputObject.Has("top_p")) 
            promptContext.top_p = inputObject.Get("top_p").As<Napi::Number>().FloatValue();
       if(inputObject.Has("temp")) 
            promptContext.temp = inputObject.Get("temp").As<Napi::Number>().FloatValue();
       if(inputObject.Has("n_batch")) 
            promptContext.n_batch = inputObject.Get("n_batch").As<Napi::Number>().Int32Value();
       if(inputObject.Has("repeat_penalty")) 
            promptContext.repeat_penalty = inputObject.Get("repeat_penalty").As<Napi::Number>().FloatValue();
       if(inputObject.Has("repeat_last_n")) 
            promptContext.repeat_last_n = inputObject.Get("repeat_last_n").As<Napi::Number>().Int32Value();
       if(inputObject.Has("context_erase")) 
            promptContext.context_erase = inputObject.Get("context_erase").As<Napi::Number>().FloatValue();
    }
    //copy to protect llmodel resources when splitting to new thread
    llmodel_prompt_context copiedPrompt = promptContext;

    std::string copiedQuestion = question;
    PromptWorkContext pc = {
        copiedQuestion,
        inference_,
        copiedPrompt,
        ""
    };
    auto threadSafeContext = new TsfnContext(env, pc);
    threadSafeContext->tsfn = Napi::ThreadSafeFunction::New(
        env,                                    // Environment
        info[2].As<Napi::Function>(),           // JS function from caller
        "PromptCallback",                       // Resource name
        0,                                      // Max queue size (0 = unlimited).
        1,                                      // Initial thread count
        threadSafeContext,                      // Context,
        FinalizerCallback,                      // Finalizer
        (void*)nullptr                          // Finalizer data
    );
    threadSafeContext->nativeThread = std::thread(threadEntry, threadSafeContext);
    return threadSafeContext->deferred_.Promise();
  }
  void NodeModelWrapper::Dispose(const Napi::CallbackInfo& info) {
    llmodel_model_destroy(inference_);
  }
  void NodeModelWrapper::SetThreadCount(const Napi::CallbackInfo& info) {
    if(info[0].IsNumber()) {
        llmodel_setThreadCount(GetInference(), info[0].As<Napi::Number>().Int64Value());
    } else {
        Napi::Error::New(info.Env(), "Could not set thread count: argument 1 is NaN").ThrowAsJavaScriptException(); 
        return;
    }
  }

  Napi::Value NodeModelWrapper::getName(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), name);
  }
  Napi::Value NodeModelWrapper::ThreadCount(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), llmodel_threadCount(GetInference()));
  }

  Napi::Value NodeModelWrapper::GetLibraryPath(const Napi::CallbackInfo& info) {
      return Napi::String::New(info.Env(),
        llmodel_get_implementation_search_path());
  }

  llmodel_model NodeModelWrapper::GetInference() {
    return inference_;
  }

//Exports Bindings
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports["LLModel"] = NodeModelWrapper::GetClass(env);
  return exports;
}



NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
