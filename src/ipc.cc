#include "ipc.h"

#include <cassert>

const char* IpcIdToString(IpcId id) {
  switch (id) {
    case IpcId::CancelRequest:
      return "$/cancelRequest";
    case IpcId::Initialized:
      return "initialized";
    case IpcId::Exit:
      return "exit";

#define CASE(name, method) \
  case IpcId::name:        \
    return method;
#include "methods.inc"
#undef CASE

    case IpcId::Unknown:
      return "$unknown";
  }

  CQUERY_UNREACHABLE("missing IpcId string name");
}

BaseIpcMessage::BaseIpcMessage(IpcId method_id) : method_id(method_id) {}

BaseIpcMessage::~BaseIpcMessage() = default;

lsRequestId BaseIpcMessage::GetRequestId() {
  return std::monostate();
}
