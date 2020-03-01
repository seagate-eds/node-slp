#include <string>
#include <vector>
#include "findsrvtypes.h"

using namespace v8;

struct FindSrvTypesBaton : Baton {
  // arguments
  String::Utf8Value namingAuthority;
  String::Utf8Value scopeList;
  // result array
  std::vector<std::string> result;

  FindSrvTypesBaton(const Arguments& args) : Baton(), namingAuthority(args[0]), scopeList(args[1]) {
    HandleScope scope;
    callback = Persistent<Function>::New(Local<Function>::Cast(args[2]));
  }

  static void work(uv_work_t* work_req) {
    FindSrvTypesBaton* baton = static_cast<FindSrvTypesBaton*>(work_req->data);
    baton->slp_error = SLPFindSrvTypes(baton->slp_handle, *baton->namingAuthority, *baton->scopeList,
      &FindSrvTypesBaton::callbackSLP, baton);
  }

  static SLPBoolean SLPCALLBACK callbackSLP(SLPHandle hSLP, const char* pcSrvTypes, SLPError errCode, void *pvCookie) {
    FindSrvTypesBaton* baton = static_cast<FindSrvTypesBaton*>(pvCookie);
    if (errCode == SLP_OK || errCode == SLP_LAST_CALL) {
      if (pcSrvTypes != NULL)
        baton->result.push_back(std::string(pcSrvTypes));
      return SLP_TRUE;
    } else {
      baton->slp_error = errCode;
      return SLP_FALSE;
    }
  }

  static void afterWork(uv_work_t* work_req) {
    HandleScope scope;
    FindSrvTypesBaton* baton = static_cast<FindSrvTypesBaton*>(work_req->data);
    if (baton->slp_error < SLP_OK) {
      Local<Value> argv[1] = { Exception::Error(String::New(slp_error_message(baton->slp_error))) };
      baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
      Local<Array> arr = Array::New(); // [ { "", ... ]
      for (unsigned int i = 0; i < baton->result.size(); i++) {
        arr->Set(i, String::New(baton->result[i].c_str()));
      }
      Local<Value> argv[2] = { Local<Value>::New(Null()), arr };
      baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
    }
    baton->callback.Dispose();
    delete baton;
  }
};

Local<Value> FindSrvTypes(const Arguments& args) {
  homerun(new FindSrvTypesBaton(args));
  return Undefined();
}
