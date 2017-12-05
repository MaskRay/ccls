#include "message_handler.h"

#include <loguru.hpp>

struct ExitHandler : MessageHandler {
  IpcId GetId() const override { return IpcId::Exit; }

  void Run(std::unique_ptr<BaseIpcMessage> request) override {
    LOG_S(INFO) << "Exiting; got IpcId::Exit";
    exit(0);
  }
};
REGISTER_MESSAGE_HANDLER(ExitHandler);
