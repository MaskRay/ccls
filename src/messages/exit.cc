#include "message_handler.h"

#include <loguru.hpp>

namespace {
struct Ipc_Exit : public IpcMessage<Ipc_Exit> {
  static const IpcId kIpcId = IpcId::Exit;
};
MAKE_REFLECT_EMPTY_STRUCT(Ipc_Exit);
REGISTER_IPC_MESSAGE(Ipc_Exit);

struct ExitHandler : MessageHandler {
  IpcId GetId() const override { return IpcId::Exit; }

  void Run(std::unique_ptr<BaseIpcMessage> request) override {
    LOG_S(INFO) << "Exiting; got IpcId::Exit";
    exit(0);
  }
};
REGISTER_MESSAGE_HANDLER(ExitHandler);
}  // namespace