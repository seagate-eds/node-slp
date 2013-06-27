#ifndef __BATON_H__
#define __BATON_H__

#include <v8.h>
#include <uv.h>
#include <slp.h>

using namespace v8;

SLPHandle acquire_handle();
void release_handle(SLPHandle handle);
void clear_handles();

struct Baton {
  uv_work_t request;
  Persistent<Function> callback;
  SLPHandle slp_handle;
  SLPError slp_error;

  Baton() { slp_handle = acquire_handle(); }
  ~Baton() { release_handle(slp_handle); }

  // static void work(uv_work_t* work_req) {}
  // static void afterWork(uv_work_t* work_req) {}
};

template <typename T> inline static void homerun(T* baton) {
  baton->request.data = baton;
  uv_queue_work(uv_default_loop(), &baton->request, &T::work, &T::afterWork);
}

const char* slp_error_message(const SLPError error);

#endif /* __BATON_H__ */