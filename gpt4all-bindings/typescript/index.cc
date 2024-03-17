#include "index.h"

Napi::Function NodeModelWrapper::GetClass(Napi::Env env)
{
    Napi::Function self = DefineClass(env, "LLModel",
                                      {InstanceMethod("type", &NodeModelWrapper::GetType),
                                       InstanceMethod("isModelLoaded", &NodeModelWrapper::IsModelLoaded),
                                       InstanceMethod("name", &NodeModelWrapper::GetName),
                                       InstanceMethod("stateSize", &NodeModelWrapper::StateSize),
                                       InstanceMethod("infer", &NodeModelWrapper::Infer),
                                       InstanceMethod("setThreadCount", &NodeModelWrapper::SetThreadCount),
                                       InstanceMethod("embed", &NodeModelWrapper::GenerateEmbedding),
                                       InstanceMethod("threadCount", &NodeModelWrapper::ThreadCount),
                                       InstanceMethod("getLibraryPath", &NodeModelWrapper::GetLibraryPath),
                                       InstanceMethod("initGpuByString", &NodeModelWrapper::InitGpuByString),
                                       InstanceMethod("hasGpuDevice", &NodeModelWrapper::HasGpuDevice),
                                       InstanceMethod("listGpu", &NodeModelWrapper::GetGpuDevices),
                                       InstanceMethod("memoryNeeded", &NodeModelWrapper::GetRequiredMemory),
                                       InstanceMethod("dispose", &NodeModelWrapper::Dispose)});
    // Keep a static reference to the constructor
    //
    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(self);
    env.SetInstanceData(constructor);
    return self;
}
Napi::Value NodeModelWrapper::GetRequiredMemory(const Napi::CallbackInfo &info)
{
    auto env = info.Env();
    return Napi::Number::New(
        env, static_cast<uint32_t>(llmodel_required_mem(GetInference(), full_model_path.c_str(), nCtx, nGpuLayers)));
}
Napi::Value NodeModelWrapper::GetGpuDevices(const Napi::CallbackInfo &info)
{
    auto env = info.Env();
    int num_devices = 0;
    auto mem_size = llmodel_required_mem(GetInference(), full_model_path.c_str(), nCtx, nGpuLayers);
    llmodel_gpu_device *all_devices = llmodel_available_gpu_devices(GetInference(), mem_size, &num_devices);
    if (all_devices == nullptr)
    {
        Napi::Error::New(env, "Unable to retrieve list of all GPU devices").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto js_array = Napi::Array::New(env, num_devices);
    for (int i = 0; i < num_devices; ++i)
    {
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
        js_gpu_device["heapSize"] = static_cast<uint32_t>(gpu_device.heapSize);
        js_gpu_device["name"] = gpu_device.name;
        js_gpu_device["vendor"] = gpu_device.vendor;

        js_array[i] = js_gpu_device;
    }
    return js_array;
}

Napi::Value NodeModelWrapper::GetType(const Napi::CallbackInfo &info)
{
    if (type.empty())
    {
        return info.Env().Undefined();
    }
    return Napi::String::New(info.Env(), type);
}

// Napi::Value NodeModelWrapper::GetContextWindowSize(const Napi::CallbackInfo &info)
// {
//     return Napi::Number::New(info.Env(), nCtx);
// }

Napi::Value NodeModelWrapper::InitGpuByString(const Napi::CallbackInfo &info)
{
    auto env = info.Env();
    size_t memory_required = static_cast<size_t>(info[0].As<Napi::Number>().Uint32Value());

    std::string gpu_device_identifier = info[1].As<Napi::String>();

    size_t converted_value;
    if (memory_required <= std::numeric_limits<size_t>::max())
    {
        converted_value = static_cast<size_t>(memory_required);
    }
    else
    {
        Napi::Error::New(env, "invalid number for memory size. Exceeded bounds for memory.")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto result = llmodel_gpu_init_gpu_device_by_string(GetInference(), converted_value, gpu_device_identifier.c_str());
    return Napi::Boolean::New(env, result);
}
Napi::Value NodeModelWrapper::HasGpuDevice(const Napi::CallbackInfo &info)
{
    return Napi::Boolean::New(info.Env(), llmodel_has_gpu_device(GetInference()));
}

NodeModelWrapper::NodeModelWrapper(const Napi::CallbackInfo &info) : Napi::ObjectWrap<NodeModelWrapper>(info)
{
    auto env = info.Env();
    auto config_object = info[0].As<Napi::Object>();

    //sets the directory where models (gguf files) are to be searched
    llmodel_set_implementation_search_path(
        config_object.Has("library_path")
        ? config_object.Get("library_path").As<Napi::String>().Utf8Value().c_str()
        : "."
    );

    std::string model_name = config_object.Get("model_name").As<Napi::String>();
    fs::path model_path = config_object.Get("model_path").As<Napi::String>().Utf8Value();
    std::string full_weight_path = (model_path / fs::path(model_name)).string();

    const char *e;
    inference_ = llmodel_model_create2(full_weight_path.c_str(), "auto", &e);
    if (!inference_)
    {
        Napi::Error::New(env, e).ThrowAsJavaScriptException();
        return;
    }
    if (GetInference() == nullptr)
    {
        std::cerr << "Tried searching libraries in \"" << llmodel_get_implementation_search_path() << "\"" << std::endl;
        std::cerr << "Tried searching for model weight in \"" << full_weight_path << "\"" << std::endl;
        std::cerr << "Do you have runtime libraries installed?" << std::endl;
        Napi::Error::New(env, "Had an issue creating llmodel object, inference is null").ThrowAsJavaScriptException();
        return;
    }

    std::string device = config_object.Get("device").As<Napi::String>();
    if (device != "cpu")
    {
        size_t mem = llmodel_required_mem(GetInference(), full_weight_path.c_str(), nCtx, nGpuLayers);

        auto success = llmodel_gpu_init_gpu_device_by_string(GetInference(), mem, device.c_str());
        if (!success)
        {
            // https://github.com/nomic-ai/gpt4all/blob/3acbef14b7c2436fe033cae9036e695d77461a16/gpt4all-bindings/python/gpt4all/pyllmodel.py#L215
            // Haven't implemented this but it is still open to contribution
            std::cout << "WARNING: Failed to init GPU\n";
        }
    }

    auto success = llmodel_loadModel(GetInference(), full_weight_path.c_str(), nCtx, nGpuLayers);
    if (!success)
    {
        Napi::Error::New(env, "Failed to load model at given path").ThrowAsJavaScriptException();
        return;
    }

    if (config_object.Has("model_type"))
    {
        type = config_object.Get("model_type").As<Napi::String>();
    }
    name = model_name.empty() ? model_path.filename().string() : model_name;
    full_model_path = full_weight_path;
    nCtx = config_object.Get("nCtx").As<Napi::Number>().Int32Value();
    nGpuLayers = config_object.Get("ngl").As<Napi::Number>().Int32Value();
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
Napi::Value NodeModelWrapper::IsModelLoaded(const Napi::CallbackInfo &info)
{
    return Napi::Boolean::New(info.Env(), llmodel_isModelLoaded(GetInference()));
}

Napi::Value NodeModelWrapper::StateSize(const Napi::CallbackInfo &info)
{
    // Implement the binding for the stateSize method
    return Napi::Number::New(info.Env(), static_cast<int64_t>(llmodel_get_state_size(GetInference())));
}

Napi::Value NodeModelWrapper::GenerateEmbedding(const Napi::CallbackInfo &info)
{
    auto env = info.Env();

    auto prefix = info[1];
    auto dimensionality = info[2].As<Napi::Number>().Int32Value();
    auto do_mean = info[3].As<Napi::Boolean>().Value();
    auto atlas = info[4].As<Napi::Boolean>().Value();
    size_t embedding_size = 0;

    // This procedure can maybe be optimized but its whatever, i have too many intermediary structures
    std::vector<std::string> text_arr;
    bool is_single_text = false;
    if (info[0].IsString())
    {
        is_single_text = true;
        text_arr.push_back(info[0].As<Napi::String>().Utf8Value());
    }
    else
    {
        auto jsarr = info[0].As<Napi::Array>();
        size_t len = jsarr.Length();
        for (size_t i = 0; i < len; ++i)
        {
            std::string str = jsarr.Get(i).As<Napi::String>().Utf8Value();
            text_arr.push_back(str);
        }
    }
    std::vector<const char *> str_ptrs;
    for (size_t i = 0; i < text_arr.size(); ++i)
        str_ptrs.push_back(text_arr[i].c_str());

    const char *err = nullptr;
    float *embeds = llmodel_embed(GetInference(), str_ptrs.data(), &embedding_size,
                                  prefix.IsUndefined() ? NULL : prefix.As<Napi::String>().Utf8Value().c_str(),
                                  dimensionality, do_mean, atlas, &err);
    if (err)
    {
        Napi::Error::New(env, strcmp(err, "(unknown error)") == 0 ? "Unknown error: sorry bud" : err)
            .ThrowAsJavaScriptException();
        delete err;
        return env.Undefined();
    }

    Napi::Float32Array js_array = Napi::Float32Array::New(env, embedding_size);
    for (size_t i = 0; i < embedding_size; ++i)
    {
        float element = *(embeds + i);
        js_array[i] = element;
    }

    llmodel_free_embedding(embeds);
    if (is_single_text)
    {
        return js_array;
    }
    else
    {
        return js_array;
    }
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
Napi::Value NodeModelWrapper::Infer(const Napi::CallbackInfo &info)
{
    auto env = info.Env();
    std::string prompt;
    if (info[0].IsString())
    {
        prompt = info[0].As<Napi::String>().Utf8Value();
    }
    else
    {
        Napi::Error::New(info.Env(), "invalid string argument").ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    if (!info[1].IsObject())
    {
        Napi::Error::New(info.Env(), "Missing Prompt Options").ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }
    // defaults copied from python bindings
    llmodel_prompt_context promptContext = {.logits = nullptr,
                                            .tokens = nullptr,
                                            .n_past = 0,
                                            .n_ctx = nCtx,
                                            .n_predict = 4096,
                                            .top_k = 40,
                                            .top_p = 0.9f,
                                            .min_p = 0.0f,
                                            .temp = 0.1f,
                                            .n_batch = 8,
                                            .repeat_penalty = 1.2f,
                                            .repeat_last_n = 10,
                                            .context_erase = 0.75};

    PromptWorkerConfig promptWorkerConfig;

    auto inputObject = info[1].As<Napi::Object>();

    // Extract and assign the properties
    if (inputObject.Has("logits") || inputObject.Has("tokens"))
    {
        Napi::Error::New(info.Env(), "Invalid input: 'logits' or 'tokens' properties are not allowed")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }
    // Assign the remaining properties
    if (inputObject.Has("nPast"))
        promptContext.n_past = inputObject.Get("nPast").As<Napi::Number>().Int32Value();
    if (inputObject.Has("nPredict"))
        promptContext.n_predict = inputObject.Get("nPredict").As<Napi::Number>().Int32Value();
    if (inputObject.Has("topK"))
        promptContext.top_k = inputObject.Get("topK").As<Napi::Number>().Int32Value();
    if (inputObject.Has("topP"))
        promptContext.top_p = inputObject.Get("topP").As<Napi::Number>().FloatValue();
    if (inputObject.Has("minP"))
        promptContext.min_p = inputObject.Get("minP").As<Napi::Number>().FloatValue();
    if (inputObject.Has("temp"))
        promptContext.temp = inputObject.Get("temp").As<Napi::Number>().FloatValue();
    if (inputObject.Has("nBatch"))
        promptContext.n_batch = inputObject.Get("nBatch").As<Napi::Number>().Int32Value();
    if (inputObject.Has("repeatPenalty"))
        promptContext.repeat_penalty = inputObject.Get("repeatPenalty").As<Napi::Number>().FloatValue();
    if (inputObject.Has("repeatLastN"))
        promptContext.repeat_last_n = inputObject.Get("repeatLastN").As<Napi::Number>().Int32Value();
    if (inputObject.Has("contextErase"))
        promptContext.context_erase = inputObject.Get("contextErase").As<Napi::Number>().FloatValue();

    if (info.Length() >= 3 && info[2].IsFunction())
    {
        promptWorkerConfig.bHasTokenCallback = true;
        promptWorkerConfig.tokenCallback = info[2].As<Napi::Function>();
    }

    // copy to protect llmodel resources when splitting to new thread
    //  llmodel_prompt_context copiedPrompt = promptContext;
    promptWorkerConfig.context = promptContext;
    promptWorkerConfig.model = GetInference();
    promptWorkerConfig.mutex = &inference_mutex;
    promptWorkerConfig.prompt = prompt;
    promptWorkerConfig.result = "";

    promptWorkerConfig.promptTemplate = inputObject.Get("promptTemplate").As<Napi::String>();
    if (inputObject.Has("special"))
    {
        promptWorkerConfig.special = inputObject.Get("special").As<Napi::Boolean>();
    }
    if (inputObject.Has("fakeReply"))
    {
        // this will be deleted in the worker
        promptWorkerConfig.fakeReply = new std::string(inputObject.Get("fakeReply").As<Napi::String>().Utf8Value());
    }
    auto worker = new PromptWorker(env, promptWorkerConfig);

    worker->Queue();

    return worker->GetPromise();
}
void NodeModelWrapper::Dispose(const Napi::CallbackInfo &info)
{
    llmodel_model_destroy(inference_);
}
void NodeModelWrapper::SetThreadCount(const Napi::CallbackInfo &info)
{
    if (info[0].IsNumber())
    {
        llmodel_setThreadCount(GetInference(), info[0].As<Napi::Number>().Int64Value());
    }
    else
    {
        Napi::Error::New(info.Env(), "Could not set thread count: argument 1 is NaN").ThrowAsJavaScriptException();
        return;
    }
}

Napi::Value NodeModelWrapper::GetName(const Napi::CallbackInfo &info)
{
    return Napi::String::New(info.Env(), name);
}
Napi::Value NodeModelWrapper::ThreadCount(const Napi::CallbackInfo &info)
{
    return Napi::Number::New(info.Env(), llmodel_threadCount(GetInference()));
}

Napi::Value NodeModelWrapper::GetLibraryPath(const Napi::CallbackInfo &info)
{
    return Napi::String::New(info.Env(), llmodel_get_implementation_search_path());
}

llmodel_model NodeModelWrapper::GetInference()
{
    return inference_;
}

// Exports Bindings
Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports["LLModel"] = NodeModelWrapper::GetClass(env);
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
