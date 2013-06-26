#include <string>
#include "findattrs.h"

using namespace v8;

struct FindAttrsBaton : Baton {
  // arguments
  String::Utf8Value urlOrServiceType;
  String::Utf8Value scopeList;
  String::Utf8Value attrIds;
  // result
  std::string result;

  FindAttrsBaton(const Arguments& args) :
    Baton(),
    urlOrServiceType(args[0]),
    scopeList(args[1]),
    attrIds(args[2]) { }

  static void work(uv_work_t* work_req) {
    FindAttrsBaton* baton = static_cast<FindAttrsBaton*>(work_req->data);
    baton->slp_error = SLPFindAttrs(baton->slp_handle, *baton->urlOrServiceType, *baton->scopeList, *baton->attrIds,
      &FindAttrsBaton::callbackSLP, baton);
  }

  static SLPBoolean SLPCALLBACK callbackSLP(SLPHandle hSLP, const char* pcAttrList, SLPError errCode, void *pvCookie) {
    FindAttrsBaton* baton = static_cast<FindAttrsBaton*>(pvCookie);
    baton->slp_error = errCode;
    if (pcAttrList != NULL)
      baton->result = pcAttrList;
    return SLP_TRUE;
  }

  static void afterWork(uv_work_t* work_req) {
    HandleScope scope;
    FindAttrsBaton* baton = static_cast<FindAttrsBaton*>(work_req->data);
    if (baton->slp_error < SLP_OK) {
      Local<Value> argv[1] = { Exception::Error(String::New(slp_error_message(baton->slp_error))) };
      baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
      Local<Value> argv[2] = { Local<Value>::New(Null()), String::New(baton->result.c_str()) };
      baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
    }
    baton->callback.Dispose();
    delete baton;
  }
};

Handle<Value> FindAttrs(const Arguments& args) {
  HandleScope scope;

  FindAttrsBaton* baton = new FindAttrsBaton(args);
  baton->request.data = baton;
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[3]));

  // launch
  homerun<FindAttrsBaton>(*baton);
  return Undefined();
}
