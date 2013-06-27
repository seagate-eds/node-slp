#include <string>
#include "dereg.h"

using namespace v8;

struct DeregBaton : Baton {
  // arguments
  String::Utf8Value srvURL;
  // result = slp_error

  DeregBaton(const Arguments& args) : Baton(), srvURL(args[0]) {
    HandleScope scope;
    callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));
  }

  static void work(uv_work_t* work_req) {
    DeregBaton* baton = static_cast<DeregBaton*>(work_req->data);
    baton->slp_error = SLPDereg(baton->slp_handle, *baton->srvURL, &DeregBaton::callbackSLP, baton);
  }

  static void SLPCALLBACK callbackSLP(SLPHandle hSLP, SLPError errCode, void *pvCookie) {
    DeregBaton* baton = static_cast<DeregBaton*>(pvCookie);
    baton->slp_error = errCode;
  }

  static void afterWork(uv_work_t* work_req) {
    HandleScope scope;
    Local<Value> argv[1];
    DeregBaton* baton = static_cast<DeregBaton*>(work_req->data);
    if (baton->slp_error < SLP_OK) {
      argv[0] = Exception::Error(String::New(slp_error_message(baton->slp_error)));
    } else {
      argv[0] = Local<Value>::New(Null());
    }
    baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    baton->callback.Dispose();
    delete baton;
  }
};

Handle<Value> Dereg(const Arguments& args) {
  homerun(new DeregBaton(args));
  return Undefined();
}
