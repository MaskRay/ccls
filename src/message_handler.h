#pragma once

#include "ipc.h"
#include "language_server_api.h"
#include "query.h"

#include <optional.h>

#include <memory>
#include <vector>

struct ClangCompleteManager;
struct CodeCompleteCache;
struct Config;
struct FileConsumerSharedState;
struct ImportManager;
struct ImportPipelineStatus;
struct IncludeComplete;
struct MultiQueueWaiter;
struct Project;
struct QueryDatabase;
struct SemanticHighlightSymbolCache;
struct TimestampManager;
struct WorkingFile;
struct WorkingFiles;

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
  MultiQueueWaiter* waiter = nullptr;
  Project* project = nullptr;
  FileConsumerSharedState* file_consumer_shared = nullptr;
  ImportManager* import_manager = nullptr;
  ImportPipelineStatus* import_pipeline_status = nullptr;
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
                    const Project* project,
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

bool ShouldIgnoreFileForIndexing(const std::string& path);
