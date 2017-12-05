#include "message_handler.h"

MessageHandler::MessageHandler() {
  // Dynamically allocate |message_handlers|, otherwise there will be static
  // initialization order races.
  if (!message_handlers)
    message_handlers = new std::vector<MessageHandler*>();
  message_handlers->push_back(this);
}

// static
std::vector<MessageHandler*>* MessageHandler::message_handlers = nullptr;