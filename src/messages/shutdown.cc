// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"
#include "pipeline.hh"
using namespace ccls;

namespace {
MethodType kMethodType = "shutdown";

struct In_Shutdown : public RequestMessage {
  MethodType GetMethodType() const override { return kMethodType; }
};
MAKE_REFLECT_STRUCT(In_Shutdown, id);
REGISTER_IN_MESSAGE(In_Shutdown);

struct Handler_Shutdown : BaseMessageHandler<In_Shutdown> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_Shutdown *request) override {
    JsonNull result;
    pipeline::Reply(request->id, result);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_Shutdown);
} // namespace
