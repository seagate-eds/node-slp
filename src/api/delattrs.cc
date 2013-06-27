#include <string>
#include "delattrs.h"

using namespace v8;

struct DelAttrsBaton : Baton {
  // arguments
  String::Utf8Value srvURL;
  String::Utf8Value attrs;
  // result = slp_error

  DelAttrsBaton(const Arguments& args) : Baton(), srvURL(args[0]), attrs(args[1]) {
    HandleScope scope;
    callback = Persistent<Function>::New(Local<Function>::Cast(args[2]));
  }

  static void work(uv_work_t* work_req) {
    DelAttrsBaton* baton = static_cast<DelAttrsBaton*>(work_req->data);
    baton->slp_error = SLPDelAttrs(baton->slp_handle, *baton->srvURL, *baton->attrs,
      &DelAttrsBaton::callbackSLP, baton);
  }

  static void SLPCALLBACK callbackSLP(SLPHandle hSLP, SLPError errCode, void *pvCookie) {
    DelAttrsBaton* baton = static_cast<DelAttrsBaton*>(pvCookie);
    baton->slp_error = errCode;
  }

  static void afterWork(uv_work_t* work_req) {
    HandleScope scope;
    Local<Value> argv[1];
    DelAttrsBaton* baton = static_cast<DelAttrsBaton*>(work_req->data);
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

Handle<Value> DelAttrs(const Arguments& args) {
  DelAttrsBaton* baton = new DelAttrsBaton(args);
  baton->request.data = baton;
  homerun<DelAttrsBaton>(*baton);
  return Undefined();
}
