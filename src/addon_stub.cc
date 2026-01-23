#include <napi.h>

Napi::Value GetActiveWindowWrapped(const Napi::CallbackInfo &info) {
  return info.Env().Null();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("getActiveWindow", Napi::Function::New(env, GetActiveWindowWrapped));
  return exports;
}

NODE_API_MODULE(win_trace, Init)
