/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

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
