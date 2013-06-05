#include "node_slp.h"

/*
  2013-06-04
  OpenSLP 1.2.1 on Linux doesn't seem to support Async mode (SLPOpen returns SLP_NOT_IMPLEMENTED).
  This module uses libuv to queue certain operations.
*/

using namespace v8;

struct ObjectCallback {
  Persistent<Function> callback;
  NodeOpenSLP* obj;
};

Handle<Value> Version(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  // XXX OpenSLP version developed for/against
  return scope.Close(String::New("1.2.1"));
}

Handle<Value> ParseSrvURL(const Arguments& args) {
  HandleScope scope;
  String::AsciiValue srvURL(args[0]);
  SLPSrvURL* pSrvURL;
  SLPError ret = SLPParseSrvURL(*srvURL, &pSrvURL);
  if (ret != SLP_OK) {
    SLPFree(pSrvURL);
    return ThrowException(Exception::Error(String::New("ParseSrvURL error")));
  }
  // build and return an object
  Local<Object> obj = Object::New();
  obj->Set(String::NewSymbol("type"), String::New(pSrvURL->s_pcSrvType));
  obj->Set(String::NewSymbol("host"), String::New(pSrvURL->s_pcHost));
  obj->Set(String::NewSymbol("port"), Integer::New(pSrvURL->s_iPort));
  obj->Set(String::NewSymbol("netFamily"), String::New(pSrvURL->s_pcNetFamily));
  obj->Set(String::NewSymbol("part"), String::New(pSrvURL->s_pcSrvPart));
  SLPFree(pSrvURL);
  return scope.Close(obj);
}

Handle<Value> Escape(const Arguments& args) {
  HandleScope scope;
  String::AsciiValue inbuf(args[0]);
  char* outBuf;
  SLPBoolean isTag = (args.Length() > 1 && args[1]->BooleanValue()) ? SLP_TRUE : SLP_FALSE;
  SLPError ret = SLPEscape(*inbuf, &outBuf, isTag);
  Local<String> result = String::New(outBuf);
  SLPFree(outBuf);
  switch (ret) {
    case SLP_OK:
      return scope.Close(result);
    case SLP_PARSE_ERROR:
      return ThrowException(Exception::Error(String::New("Parse error")));
    default:
      return ThrowException(Exception::Error(String::New("Escape error")));
  }
}

Handle<Value> Unescape(const Arguments& args) {
  HandleScope scope;
  String::AsciiValue inbuf(args[0]);
  char* outBuf;
  SLPError ret = SLPUnescape(*inbuf, &outBuf, args[1]->BooleanValue() ? SLP_TRUE : SLP_FALSE);
  Local<String> result = String::New(outBuf);
  SLPFree(outBuf);
  switch (ret) {
    case SLP_OK:
      return scope.Close(result);
    case SLP_PARSE_ERROR:
      return ThrowException(Exception::Error(String::New("Parse error")));
    default:
      return ThrowException(Exception::Error(String::New("Unescape error")));
  }
}

Handle<Value> GetSetProperty(const Arguments& args) {
  HandleScope scope;
  String::AsciiValue name(args[0]);
  if (args.Length() > 1) {
    String::Utf8Value value(args[1]);
    SLPSetProperty(*name, *value);
    return Undefined();
  } else {
    const char* result = SLPGetProperty(*name);
    return (result == NULL) ? Null() : scope.Close(String::New(result));
  }
}

Handle<Value> GetRefreshInterval(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  return scope.Close(Integer::New(SLPGetRefreshInterval()));
}

extern "C" void init(Handle<Object> target) {
  target->SetAccessor(String::NewSymbol("version"), Version);
  target->SetAccessor(String::NewSymbol("refreshInterval"), GetRefreshInterval);
  // set addon methods
  NODE_SET_METHOD(target, "parseSrvUrl", ParseSrvURL);
  NODE_SET_METHOD(target, "escape", Escape);
  NODE_SET_METHOD(target, "unescape", Unescape);
  NODE_SET_METHOD(target, "property", GetSetProperty);
  // set NodeOpenSLP class
  NodeOpenSLP::Init(target);
}

NODE_MODULE(slp, init);

// NodeOpenSLP class imp

void NodeOpenSLP::Init(Handle<Object> target) {
  Local<FunctionTemplate> tpl = FunctionTemplate::New(NewInstance);
  tpl->SetClassName(String::NewSymbol("OpenSLP"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  NODE_SET_PROTOTYPE_METHOD(tpl, "reg", Reg);
  NODE_SET_PROTOTYPE_METHOD(tpl, "dereg", Dereg);
  NODE_SET_PROTOTYPE_METHOD(tpl, "delAttrs", DelAttrs);
  NODE_SET_PROTOTYPE_METHOD(tpl, "findScopes", FindScopes);
  NODE_SET_PROTOTYPE_METHOD(tpl, "findSrvs", FindSrvs);
  NODE_SET_PROTOTYPE_METHOD(tpl, "findSrvTypes", FindSrvTypes);
  NODE_SET_PROTOTYPE_METHOD(tpl, "findAttrs", FindAttrs);
  tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("lastError"), GetLastError);
  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("OpenSLP"), constructor);
}

Handle<Value> NodeOpenSLP::NewInstance(const Arguments& args) {
  HandleScope scope;
  SLPHandle handle;
  SLPError err = SLPOpen(args[0]->IsUndefined() ? NULL : *String::AsciiValue(args[0]), SLP_FALSE, &handle);
  if (err != SLP_OK) {
    // FIXME add err to the exception message
    // return ThrowException(Exception::Error(String::New("SLPOpen failed")));
  }
  NodeOpenSLP* obj = new NodeOpenSLP(handle);
  obj->m_lasterr = err;
  obj->Wrap(args.This());
  return args.This();
}

NodeOpenSLP::NodeOpenSLP(SLPHandle _handle) : node::ObjectWrap(), m_handle(_handle) {
}

NodeOpenSLP::~NodeOpenSLP() {
  if (m_handle) {
    SLPClose(m_handle);
    m_handle = NULL;
  }
}

void fcall1(const Persistent<Function> &func, const Local<Value> &value) {
  const unsigned argc = 1;
  Local<Value> argv[argc] = { value };
  func->Call(Context::GetCurrent()->Global(), argc, argv);
}

void fcall2(const Persistent<Function> &func, const Local<Value> &value1, const Local<Value> &value2) {
  const unsigned argc = 2;
  Local<Value> argv[argc] = { value1, value2 };
  func->Call(Context::GetCurrent()->Global(), argc, argv);
}

// SLPRegReport
void SLPCALLBACK nodeSLPRegReport(SLPHandle hSLP, SLPError errCode, void *pvCookie) {
  ObjectCallback* oc = static_cast<ObjectCallback*>(pvCookie);
  // TODO pass the errCode
  fcall1(oc->callback, errCode == SLP_OK ? Local<Value>(*Undefined()) : Exception::Error(String::New("OpenSLP error")));
  oc->callback.Dispose();
  delete oc;
}

// SLPSrvURLCallback
SLPBoolean SLPCALLBACK nodeSLPSrvURLCallback(SLPHandle hSLP, const char* pcSrvURL,
  unsigned short sLifetime, SLPError errCode, void *pvCookie) {
  fprintf(stderr, "%s %d\n", pcSrvURL, errCode);
  ObjectCallback* oc = static_cast<ObjectCallback*>(pvCookie);
  switch (errCode) {
    case SLP_OK:
      // ask for more data
      fcall2(oc->callback, Local<Value>(*Undefined()), String::New(pcSrvURL));
      return SLP_TRUE;
    case SLP_LAST_CALL:
      // no more data
      fcall2(oc->callback, Local<Value>(*Undefined()), String::New(pcSrvURL));
      oc->callback.Dispose();
      delete oc;
      return SLP_FALSE;
    default:
      // anything else is an error, stop asking for more data
      fcall1(oc->callback, Exception::Error(String::New("OpenSLP error")));
      oc->callback.Dispose();
      delete oc;
      return SLP_FALSE;
  }
}

// SLPSrvTypeCallback
SLPBoolean SLPCALLBACK nodeSLPSrvTypeCallback(SLPHandle hSLP, const char* pcSrvTypes,
  SLPError errCode, void *pvCookie) {
  return SLP_TRUE;
}

// SLPAttrCallback
SLPBoolean SLPCALLBACK nodeSLPAttrCallback(SLPHandle hSLP, const char* pcAttrList,
  SLPError errCode, void *pvCookie) {
  return SLP_TRUE;
}

Handle<Value> NodeOpenSLP::Reg(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue srvURL(args[0]);
  String::AsciiValue srvType(args[2]);
  String::AsciiValue attrs(args[3]);
  obj->m_lasterr = SLPReg(obj->m_handle, *srvURL, args[1]->Uint32Value(),
    *srvType, *attrs, SLP_TRUE, &nodeSLPRegReport, obj);
  return scope.Close(Integer::New(obj->m_lasterr));
}

Handle<Value> NodeOpenSLP::Dereg(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue srvURL(args[0]);
  obj->m_lasterr = SLPDereg(obj->m_handle, *srvURL, &nodeSLPRegReport, obj);
  return scope.Close(Integer::New(obj->m_lasterr));
}

Handle<Value> NodeOpenSLP::DelAttrs(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue srvURL(args[0]);
  String::AsciiValue attrs(args[1]);
  obj->m_lasterr = SLPDelAttrs(obj->m_handle, *srvURL, *attrs, &nodeSLPRegReport, obj);
  return scope.Close(Integer::New(obj->m_lasterr));
}

Handle<Value> NodeOpenSLP::FindScopes(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  char* scopeList;
  obj->m_lasterr = SLPFindScopes(obj->m_handle, &scopeList);
  Local<String> result = String::New(scopeList);
  SLPFree(scopeList);
  return scope.Close(result);
}

Handle<Value> NodeOpenSLP::FindSrvs(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue serviceType(args[0]);
  String::AsciiValue scopeList(args[1]);
  String::AsciiValue searchFilter(args[2]);

  return scope.Close(Integer::New(obj->m_lasterr));
  // uv_queue_work(uv_default_loop(), work_req, ZCtx::Process, ZCtx::After);

  // ObjectCallback* oc = new ObjectCallback();
  // oc->obj = obj;
  // oc->callback = Persistent<Function>::New(Local<Function>::Cast(args[3]));
  // obj->m_lasterr = SLPFindSrvs(obj->m_handle, *serviceType, *scopeList, *searchFilter,
  //   &nodeSLPSrvURLCallback, oc);
  // return scope.Close(Integer::New(obj->m_lasterr));
}

Handle<Value> NodeOpenSLP::FindSrvTypes(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue namingAuthority(args[0]);
  String::AsciiValue scopeList(args[1]);
  obj->m_lasterr = SLPFindSrvTypes(obj->m_handle, *namingAuthority, *scopeList,
    &nodeSLPSrvTypeCallback, obj);
  return scope.Close(Integer::New(obj->m_lasterr));
}

Handle<Value> NodeOpenSLP::FindAttrs(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue urlOrServiceType(args[0]);
  String::AsciiValue scopeList(args[1]);
  String::AsciiValue attrIds(args[2]);
  obj->m_lasterr = SLPFindAttrs(obj->m_handle, *urlOrServiceType, *scopeList, *attrIds,
    &nodeSLPAttrCallback, obj);
  return scope.Close(Integer::New(obj->m_lasterr));
}

Handle<Value> NodeOpenSLP::GetLastError(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(info.This());
  return scope.Close(Integer::New(obj->m_lasterr));
}
