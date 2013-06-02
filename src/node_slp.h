#ifndef __NODE_SLP_H__
#define __NODE_SLP_H__

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif

#include <v8.h>
#include <node.h>
#include <slp.h>

class NodeOpenSLP : public node::ObjectWrap {
public:
  static void Init(v8::Handle<v8::Object> target);

private:
  static v8::Handle<v8::Value> NewInstance(const v8::Arguments& args);

  NodeOpenSLP(SLPHandle _handle);
  ~NodeOpenSLP();

  static v8::Handle<v8::Value> Reg(const v8::Arguments& args);
  static v8::Handle<v8::Value> Dereg(const v8::Arguments& args);
  static v8::Handle<v8::Value> DelAttrs(const v8::Arguments& args);
  static v8::Handle<v8::Value> FindScopes(const v8::Arguments& args);
  static v8::Handle<v8::Value> FindSrvs(const v8::Arguments& args);
  static v8::Handle<v8::Value> FindSrvTypes(const v8::Arguments& args);
  static v8::Handle<v8::Value> FindAttrs(const v8::Arguments& args);

  SLPHandle m_handle;
};

#endif /* __NODE_SLP_H__ */