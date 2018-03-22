#pragma once

#include "lsp.h"
#include "method.h"
#include "query.h"

#include <optional.h>

#include <memory>
#include <vector>

struct ClangCompleteManager;
struct CodeCompleteCache;
struct Config;
class DiagnosticsEngine;
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

struct Out_CqueryPublishSemanticHighlighting
    : public lsOutMessage<Out_CqueryPublishSemanticHighlighting> {
  struct Symbol {
    int stableId = 0;
    lsSymbolKind parentKind;
    lsSymbolKind kind;
    StorageClass storage;
    std::vector<lsRange> ranges;
  };
  struct Params {
    lsDocumentUri uri;
    std::vector<Symbol> symbols;
  };
  std::string method = "$cquery/publishSemanticHighlighting";
  Params params;
};
MAKE_REFLECT_STRUCT(Out_CqueryPublishSemanticHighlighting::Symbol,
                    stableId,
                    parentKind,
                    kind,
                    storage,
                    ranges);
MAKE_REFLECT_STRUCT(Out_CqueryPublishSemanticHighlighting::Params,
                    uri,
                    symbols);
MAKE_REFLECT_STRUCT(Out_CqueryPublishSemanticHighlighting,
                    jsonrpc,
                    method,
                    params);

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
  DiagnosticsEngine* diag_engine = nullptr;
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

  virtual MethodType GetMethodType() const = 0;
  virtual void Run(std::unique_ptr<InMessage> message) = 0;

  static std::vector<MessageHandler*>* message_handlers;

 protected:
  MessageHandler();
};

template <typename TMessage>
struct BaseMessageHandler : MessageHandler {
  virtual void Run(TMessage* message) = 0;

  // MessageHandler:
  void Run(std::unique_ptr<InMessage> message) override {
    Run(static_cast<TMessage*>(message.get()));
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
