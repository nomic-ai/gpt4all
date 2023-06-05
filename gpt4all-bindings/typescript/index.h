#include <napi.h>
#include "llmodel.h"
#include <iostream>
#include "llmodel_c.h" 
#include "prompt.h"
#include <atomic>
class NodeModelWrapper: public Napi::ObjectWrap<NodeModelWrapper> {
public:
  NodeModelWrapper(const Napi::CallbackInfo &);
  ~NodeModelWrapper();
  void Ping(const Napi::CallbackInfo &);
  Napi::Value getType(const Napi::CallbackInfo& info);
  Napi::Value IsModelLoaded(const Napi::CallbackInfo& info);
  Napi::Value StateSize(const Napi::CallbackInfo& info);
  Napi::Value Prompt(const Napi::CallbackInfo& info);
  void SetThreadCount(const Napi::CallbackInfo& info);
  Napi::Value getName(const Napi::CallbackInfo& info);
  Napi::Value ThreadCount(const Napi::CallbackInfo& info);
  static Napi::Function GetClass(Napi::Env);
  llmodel_model GetInference();
private:
  std::shared_ptr<llmodel_model> inference_;
  // Had to use this instead of the c library in order 
  // set the type of the model loaded.
  // causes side effect: type is mutated;
  //llmodel_model create_model_set_type(const char* c_weights_path);
  std::string type;
  std::string name;
  static Napi::FunctionReference constructor;
  // static strings seem to not capture. this is greate because it removes the need for 
  // capturing stdout and rather capture from this static variable
  static std::string _res;

};
