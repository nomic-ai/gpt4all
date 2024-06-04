#include "llmodel.h"
#include "llmodel_c.h"
#include "prompt.h"
#include <atomic>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <napi.h>
#include <set>

namespace fs = std::filesystem;

class NodeModelWrapper : public Napi::ObjectWrap<NodeModelWrapper>
{

  public:
    NodeModelWrapper(const Napi::CallbackInfo &);
    Napi::Value Load(const Napi::CallbackInfo &info);
    Napi::Value InitGpu(const Napi::CallbackInfo &info);
    /**
     * Prompting the model. This entails spawning a new thread and adding the response tokens
     * into a thread local string variable.
     */
    Napi::Value Infer(const Napi::CallbackInfo &info);
    Napi::Value Embed(const Napi::CallbackInfo &info);
    Napi::Value IsModelLoaded(const Napi::CallbackInfo &info);
    Napi::Value GetType(const Napi::CallbackInfo &info);
    Napi::Value GetName(const Napi::CallbackInfo &info);
    Napi::Value GetStateSize(const Napi::CallbackInfo &info);
    void SetThreadCount(const Napi::CallbackInfo &info);
    Napi::Value GetThreadCount(const Napi::CallbackInfo &info);
    /*
     * The path that is used to search for the dynamic libraries
     */
    Napi::Value GetLibraryPath(const Napi::CallbackInfo &info);
    Napi::Value GetGpuDevices(const Napi::CallbackInfo &info);
    Napi::Value GetRequiredMemory(const Napi::CallbackInfo &info);
    void Dispose(const Napi::CallbackInfo &info);
    /**
     * Creates the LLModel class
     */
    static Napi::Function GetClass(Napi::Env);
    llmodel_model GetInference();

  private:
    /**
     * The underlying inference that interfaces with the C interface
     */
    llmodel_model inference_;

    std::mutex inference_mutex;

    std::string model_type;
    std::string model_name;
    std::string model_file;
    std::string backend;
    int n_ctx{};
    int n_gpu_layers{};
};
