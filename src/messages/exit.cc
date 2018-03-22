#include "message_handler.h"

#include <loguru.hpp>

namespace {
struct In_Exit : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType_Exit; }
};
MAKE_REFLECT_EMPTY_STRUCT(In_Exit);
REGISTER_IN_MESSAGE(In_Exit);

struct Handler_Exit : MessageHandler {
  MethodType GetMethodType() const override { return kMethodType_Exit; }

  void Run(std::unique_ptr<InMessage> request) override {
    LOG_S(INFO) << "Exiting; got exit message";
    exit(0);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_Exit);
}  // namespace
