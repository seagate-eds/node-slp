#include <string>
#include "findattrs.h"

using namespace v8;

struct FindAttrsBaton : Baton {
  // arguments
  String::Utf8Value urlOrServiceType;
  String::Utf8Value scopeList;
  String::Utf8Value attrIds;
  // result
  std::vector< std::string > result;

  FindAttrsBaton(const Nan::FunctionCallbackInfo<v8::Value>& info) : Baton(), urlOrServiceType(info[0]), scopeList(info[1]), attrIds(info[2]) {
    callback = new Nan::Callback(Nan::To<Function>(info[3]).ToLocalChecked());
  }

  static void work(uv_work_t* work_req) {
    FindAttrsBaton* baton = static_cast<FindAttrsBaton*>(work_req->data);
    baton->slp_error = SLPFindAttrs(baton->slp_handle, *baton->urlOrServiceType, *baton->scopeList, *baton->attrIds,
      &FindAttrsBaton::callbackSLP, baton);
  }

  static SLPBoolean SLPCALLBACK callbackSLP(SLPHandle hSLP, const char* pcAttrList, SLPError errCode, void *pvCookie) {
    FindAttrsBaton* baton = static_cast<FindAttrsBaton*>(pvCookie);
    baton->slp_error = errCode;
    if (errCode == SLP_OK || errCode == SLP_LAST_CALL) {
        if (pcAttrList != NULL) {
            baton->result.push_back(std::string(pcAttrList));
        }
        return SLP_TRUE;
    } else {
        baton->slp_error = errCode;
        return SLP_FALSE;
    }
  }

  static void afterWork(uv_work_t* work_req, int status) {
    Nan::HandleScope scope;
    FindAttrsBaton* baton = static_cast<FindAttrsBaton*>(work_req->data);
    if (baton->slp_error < SLP_OK) {
      Local<Value> argv[1] = { Exception::Error(Nan::New<String>(slp_error_message(baton->slp_error)).ToLocalChecked()) };
      baton->callback->Call(1, argv);
    } else {
      Local<Array> arr = Nan::New<Array>();
      for (unsigned int i = 0; i < baton->result.size(); i++) {
          Local<String> obj = Nan::New<String>(baton->result[i].c_str()).ToLocalChecked();
          arr->Set(i, obj);
      }
      Local<Value> argv[2] = { Nan::Null(), arr };

      baton->callback->Call(2, argv);
    }
    delete baton->callback;
    delete baton;
  }
};

void FindAttrs(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  homerun(new FindAttrsBaton(info));
}
