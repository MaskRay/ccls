// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"

namespace {
struct In_Exit : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType_Exit; }
};
MAKE_REFLECT_EMPTY_STRUCT(In_Exit);
REGISTER_IN_MESSAGE(In_Exit);

struct Handler_Exit : MessageHandler {
  MethodType GetMethodType() const override { return kMethodType_Exit; }

  void Run(std::unique_ptr<InMessage> request) override { exit(0); }
};
REGISTER_MESSAGE_HANDLER(Handler_Exit);
} // namespace
