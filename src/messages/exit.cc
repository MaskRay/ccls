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
