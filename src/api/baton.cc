#include "baton.h"
#include <list>

const char* slp_error_message(const SLPError error) {
  switch (error) {
    case SLP_LANGUAGE_NOT_SUPPORTED:
      return "No DA or SA has service advertisement or attribute information in the language requested";
    case SLP_PARSE_ERROR:
      return "The SLP message was rejected by a remote SLP agent";
    case SLP_INVALID_REGISTRATION:
      return "Registration rejected by all DAs because of a malformed URL or attributes";
    case SLP_SCOPE_NOT_SUPPORTED:
      return "The SA request did not specify one or more allowable scopes";
    case SLP_AUTHENTICATION_ABSENT:
      return "The UA or SA failed to send an authenticator for requests or registrations in a protected scope";
    case SLP_AUTHENTICATION_FAILED:
      return "Authentication on an SLP message failed";
    case SLP_INVALID_UPDATE:
      return "An update for a non-existing registration was issued, or the update includes a service type or scope different than that in the initial registration";
    case SLP_REFRESH_REJECTED:
      return "The SA attempted to refresh a registration more frequently than the minimum refresh interval";
    case SLP_NOT_IMPLEMENTED:
      return "Unimplemented feature";
    case SLP_BUFFER_OVERFLOW:
      return "An outgoing request overflowed the maximum network MTU size";
    case SLP_NETWORK_TIMED_OUT:
      return "Network time out";
    case SLP_NETWORK_INIT_FAILED:
      return "Network cannot initialize properly";
    case SLP_MEMORY_ALLOC_FAILED:
      return "Out of memory";
    case SLP_PARAMETER_BAD:
      return "Bad parameter";
    case SLP_NETWORK_ERROR:
      return "Networking failed during a normal operation";
    case SLP_INTERNAL_SYSTEM_ERROR:
      return "Internal system error";
    case SLP_HANDLE_IN_USE:
      return "Handle in use";
    case SLP_TYPE_ERROR:
      return "Type error";
    default:
      return "Unkown error ";
  }
}

std::list<SLPHandle> free_handles;

SLPHandle acquire_handle() {
  SLPHandle handle = NULL;
  if (free_handles.empty()) {
    SLPOpen(NULL, SLP_FALSE, &handle);
  } else {
    handle = free_handles.front();
    free_handles.pop_front();
  }
  return handle;
}

void release_handle(SLPHandle handle) {
  free_handles.push_back(handle);
}

void clear_handles() {
  for (std::list<SLPHandle>::iterator it = free_handles.begin(); it != free_handles.end(); ++it) {
    SLPClose(*it);
  }
}
