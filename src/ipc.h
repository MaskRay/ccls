#pragma once

#include "serializer.h"
#include "utils.h"

#include <string>

using lsRequestId = std::variant<std::monostate, int64_t, std::string>;

enum class IpcId : int {
  // Language server specific requests.
  CancelRequest = 0,
  Initialized,
  Exit,

#define CASE(x, _) x,
  #include "methods.inc"
#undef CASE

  // Internal implementation detail.
  Unknown,
};
MAKE_ENUM_HASHABLE(IpcId)
MAKE_REFLECT_TYPE_PROXY(IpcId)
const char* IpcIdToString(IpcId id);

struct BaseIpcMessage {
  const IpcId method_id;
  BaseIpcMessage(IpcId method_id);
  virtual ~BaseIpcMessage();

  virtual lsRequestId GetRequestId();

  template <typename T>
  T* As() {
    assert(method_id == T::kIpcId);
    return static_cast<T*>(this);
  }
};

template <typename T>
struct RequestMessage : public BaseIpcMessage {
  // number | string, actually no null
  lsRequestId id;
  RequestMessage() : BaseIpcMessage(T::kIpcId) {}

  lsRequestId GetRequestId() override { return id; }
};

// NotificationMessage does not have |id|.
template <typename T>
struct NotificationMessage : public BaseIpcMessage {
  NotificationMessage() : BaseIpcMessage(T::kIpcId) {}
};
