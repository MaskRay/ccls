#pragma once

#include "cache_loader.h"
#include "clang_complete.h"
#include "code_complete_cache.h"
#include "config.h"
#include "import_manager.h"
#include "include_complete.h"
#include "ipc_manager.h"
#include "project.h"
#include "query.h"
#include "semantic_highlight_symbol_cache.h"
#include "threaded_queue.h"
#include "timestamp_manager.h"
#include "working_files.h"

// Usage:
//
//  struct FooHandler : MessageHandler {
//    // ...
//  };
//  REGISTER_MESSAGE_HANDLER(FooHandler);
//
// Then there will be a global FooHandler instance in
// |MessageHandler::message_handlers|.

#define REGISTER_MESSAGE_HANDLER(type) \
  static type type##message_handler_instance_;

struct MessageHandler {
  Config* config = nullptr;
  QueryDatabase* db = nullptr;
  bool* exit_when_idle = nullptr;
  MultiQueueWaiter* waiter = nullptr;
  QueueManager* queue = nullptr;
  Project* project = nullptr;
  FileConsumer::SharedState* file_consumer_shared = nullptr;
  ImportManager* import_manager = nullptr;
  TimestampManager* timestamp_manager = nullptr;
  SemanticHighlightSymbolCache* semantic_cache = nullptr;
  WorkingFiles* working_files = nullptr;
  ClangCompleteManager* clang_complete = nullptr;
  IncludeComplete* include_complete = nullptr;
  CodeCompleteCache* global_code_complete_cache = nullptr;
  CodeCompleteCache* non_global_code_complete_cache = nullptr;
  CodeCompleteCache* signature_cache = nullptr;

  virtual IpcId GetId() const = 0;
  virtual void Run(std::unique_ptr<BaseIpcMessage> message) = 0;

  static std::vector<MessageHandler*>* message_handlers;

 protected:
  MessageHandler();
};

template <typename TMessage>
struct BaseMessageHandler : MessageHandler {
  virtual void Run(TMessage* message) = 0;

  // MessageHandler:
  IpcId GetId() const override { return TMessage::kIpcId; }
  void Run(std::unique_ptr<BaseIpcMessage> message) override {
    Run(message->As<TMessage>());
  }
};

bool FindFileOrFail(QueryDatabase* db,
                    optional<lsRequestId> id,
                    const std::string& absolute_path,
                    QueryFile** out_query_file,
                    QueryFileId* out_file_id = nullptr);

void EmitInactiveLines(WorkingFile* working_file,
                       const std::vector<Range>& inactive_regions);

void EmitSemanticHighlighting(QueryDatabase* db,
                              SemanticHighlightSymbolCache* semantic_cache,
                              WorkingFile* working_file,
                              QueryFile* file);