#include <utility>
#include <string>
#include <vector>
#include "findsrvs.h"

using namespace v8;

struct FindSrvsBaton : Baton {
  // arguments
  String::Utf8Value serviceType;
  String::Utf8Value scopeList;
  String::Utf8Value searchFilter;
  // result array
  std::vector< std::pair<std::string, unsigned short> > result;

  FindSrvsBaton(const Arguments& args) : Baton(), serviceType(args[0]), scopeList(args[1]), searchFilter(args[2]) {
    HandleScope scope;
    callback = Persistent<Function>::New(Local<Function>::Cast(args[3]));
  }

  static void work(uv_work_t* work_req) {
    FindSrvsBaton* baton = static_cast<FindSrvsBaton*>(work_req->data);
    baton->slp_error = SLPFindSrvs(baton->slp_handle, *baton->serviceType, *baton->scopeList, *baton->searchFilter,
      &FindSrvsBaton::callbackSLP, baton);
  }

  static SLPBoolean SLPCALLBACK callbackSLP(SLPHandle hSLP, const char* pcSrvURL, unsigned short sLifetime, SLPError errCode, void *pvCookie) {
    FindSrvsBaton* baton = static_cast<FindSrvsBaton*>(pvCookie);
    if (errCode == SLP_OK || errCode == SLP_LAST_CALL) {
      if (pcSrvURL != NULL)
        baton->result.push_back(std::make_pair(std::string(pcSrvURL), sLifetime));
      return SLP_TRUE;
    } else {
      baton->slp_error = errCode;
      return SLP_FALSE;
    }
  }

  static void afterWork(uv_work_t* work_req) {
    HandleScope scope;
    FindSrvsBaton* baton = static_cast<FindSrvsBaton*>(work_req->data);
    if (baton->slp_error < SLP_OK) {
      Local<Value> argv[1] = { Exception::Error(String::New(slp_error_message(baton->slp_error))) };
      baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
      Local<Array> arr = Array::New(); // [ { url: "", lifetime: 0 }, ... ]
      for (unsigned int i = 0; i < baton->result.size(); i++) {
        Local<Object> obj = Object::New();
        obj->Set(String::NewSymbol("url"), String::New(baton->result[i].first.c_str()));
        obj->Set(String::NewSymbol("lifetime"), Integer::New(baton->result[i].second));
        arr->Set(i, obj);
      }
      Local<Value> argv[2] = { Local<Value>::New(Null()), arr };
      baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
    }
    baton->callback.Dispose();
    delete baton;
  }
};

Handle<Value> FindSrvs(const Arguments& args) {
  homerun(new FindSrvsBaton(args));
  return Undefined();
}
