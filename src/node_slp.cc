// goo.gl/VOnhG
#include "node_slp.h"

/*
  If an slp function returns SLP_HANDLE_IN_USE we must enqueue this request for later use.
  Actually, all slp requests requiring a handle must be enqueued.

  Yes, this module ignores SLP_NOT_IMPLEMENTED.
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
  SLPError ret = SLPEscape(*inbuf, &outBuf, args[1]->BooleanValue() ? SLP_TRUE : SLP_FALSE);
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

Handle<Value> GetProperty(const Arguments& args) {
  HandleScope scope;
  String::AsciiValue name(args[0]);
  const char* ret = SLPGetProperty(*name);
  return scope.Close(String::New(ret));
}

Handle<Value> SetProperty(const Arguments& args) {
  HandleScope scope;
  String::AsciiValue name(args[0]);
  String::Utf8Value value(args[1]);
  SLPSetProperty(*name, *value);
  return scope.Close(Undefined());
}

Handle<Value> GetRefreshInterval(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  return scope.Close(Integer::New(SLPGetRefreshInterval()));
}

extern "C" void init(Handle<Object> target) {
  target->SetAccessor(String::NewSymbol("version"), Version);
  target->SetAccessor(String::NewSymbol("refresh_interval"), GetRefreshInterval);
  // set addon methods
  NODE_SET_METHOD(target, "parse_srv_url", ParseSrvURL);
  NODE_SET_METHOD(target, "escape", Escape);
  NODE_SET_METHOD(target, "unescape", Unescape);
  NODE_SET_METHOD(target, "get_property", GetProperty);
  NODE_SET_METHOD(target, "set_property", SetProperty);
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
  NODE_SET_PROTOTYPE_METHOD(tpl, "del_attrs", DelAttrs);
  NODE_SET_PROTOTYPE_METHOD(tpl, "find_scopes", FindScopes);
  NODE_SET_PROTOTYPE_METHOD(tpl, "find_srvs", FindSrvs);
  NODE_SET_PROTOTYPE_METHOD(tpl, "find_srv_types", FindSrvTypes);
  NODE_SET_PROTOTYPE_METHOD(tpl, "find_attrs", FindAttrs);
  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("OpenSLP"), constructor);
}

Handle<Value> NodeOpenSLP::NewInstance(const Arguments& args) {
  HandleScope scope;
  SLPHandle handle;
  SLPError err = SLPOpen(args[0]->IsUndefined() ? NULL : *String::AsciiValue(args[0]), SLP_TRUE, &handle);
  if (err != SLP_OK) {
    // FIXME add err to the exception message
    return ThrowException(Exception::TypeError(String::New("SLPOpen failed")));
  }
  NodeOpenSLP* obj = new NodeOpenSLP(handle);
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
  SLPError ret = SLPReg(obj->m_handle, *srvURL, args[1]->Uint32Value(),
    *srvType, *attrs, SLP_TRUE, &nodeSLPRegReport, obj);
  return scope.Close(Integer::New(ret));
}

Handle<Value> NodeOpenSLP::Dereg(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue srvURL(args[0]);
  SLPError ret = SLPDereg(obj->m_handle, *srvURL, &nodeSLPRegReport, obj);
  return scope.Close(Integer::New(ret));
}

Handle<Value> NodeOpenSLP::DelAttrs(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue srvURL(args[0]);
  String::AsciiValue attrs(args[1]);
  SLPError ret = SLPDelAttrs(obj->m_handle, *srvURL, *attrs, &nodeSLPRegReport, obj);
  return scope.Close(Integer::New(ret));
}

Handle<Value> NodeOpenSLP::FindScopes(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  char* scopeList;
  SLPError ret = SLPFindScopes(obj->m_handle, &scopeList);
  // XXX schedule a callback to return "scopeList"
  SLPFree(scopeList);
  return scope.Close(Integer::New(ret));
}

Handle<Value> NodeOpenSLP::FindSrvs(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue serviceType(args[0]);
  String::AsciiValue scopeList(args[1]);
  String::AsciiValue searchFilter(args[2]);
  SLPError ret = SLPFindSrvs(obj->m_handle, *serviceType, *scopeList, *searchFilter,
    &nodeSLPSrvURLCallback, obj);
  return scope.Close(Integer::New(ret));
}

Handle<Value> NodeOpenSLP::FindSrvTypes(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue namingAuthority(args[0]);
  String::AsciiValue scopeList(args[1]);
  SLPError ret = SLPFindSrvTypes(obj->m_handle, *namingAuthority, *scopeList,
    &nodeSLPSrvTypeCallback, obj);
  return scope.Close(Integer::New(ret));
}

Handle<Value> NodeOpenSLP::FindAttrs(const Arguments& args) {
  HandleScope scope;
  NodeOpenSLP* obj = node::ObjectWrap::Unwrap<NodeOpenSLP>(args.This());
  String::AsciiValue urlOrServiceType(args[0]);
  String::AsciiValue scopeList(args[1]);
  String::AsciiValue attrIds(args[2]);
  SLPError ret = SLPFindAttrs(obj->m_handle, *urlOrServiceType, *scopeList, *attrIds,
    &nodeSLPAttrCallback, obj);
  return scope.Close(Integer::New(ret));
}
