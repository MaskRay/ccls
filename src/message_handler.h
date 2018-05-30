#pragma once

#include "lru_cache.h"
#include "lsp.h"
#include "match.h"
#include "method.h"
#include "query.h"

#include <optional>
#include <memory>
#include <unordered_map>
#include <vector>

struct ClangCompleteManager;
struct CodeCompleteCache;
struct Config;
class DiagnosticsEngine;
struct VFS;
struct ImportManager;
struct IncludeComplete;
struct MultiQueueWaiter;
struct Project;
struct DB;
struct WorkingFile;
struct WorkingFiles;

// Caches symbols for a single file for semantic highlighting to provide
// relatively stable ids. Only supports xxx files at a time.
struct SemanticHighlightSymbolCache {
  struct Entry {
    SemanticHighlightSymbolCache* all_caches_ = nullptr;

    // The path this cache belongs to.
    std::string path;
    // Detailed symbol name to stable id.
    using TNameToId = std::unordered_map<std::string, int>;
    TNameToId detailed_type_name_to_stable_id;
    TNameToId detailed_func_name_to_stable_id;
    TNameToId detailed_var_name_to_stable_id;

    Entry(SemanticHighlightSymbolCache* all_caches, const std::string& path);

    std::optional<int> TryGetStableId(SymbolKind kind,
                                 const std::string& detailed_name);
    int GetStableId(SymbolKind kind, const std::string& detailed_name);

    TNameToId* GetMapForSymbol_(SymbolKind kind);
  };

  constexpr static int kCacheSize = 10;
  LruCache<std::string, Entry> cache_;
  uint32_t next_stable_id_ = 0;
  std::unique_ptr<GroupMatch> match_;

  SemanticHighlightSymbolCache();
  void Init();
  std::shared_ptr<Entry> GetCacheForFile(const std::string& path);
};

struct Out_CclsPublishSemanticHighlighting
    : public lsOutMessage<Out_CclsPublishSemanticHighlighting> {
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
  std::string method = "$ccls/publishSemanticHighlighting";
  Params params;
};
MAKE_REFLECT_STRUCT(Out_CclsPublishSemanticHighlighting::Symbol,
                    stableId,
                    parentKind,
                    kind,
                    storage,
                    ranges);
MAKE_REFLECT_STRUCT(Out_CclsPublishSemanticHighlighting::Params,
                    uri,
                    symbols);
MAKE_REFLECT_STRUCT(Out_CclsPublishSemanticHighlighting,
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
  DB* db = nullptr;
  MultiQueueWaiter* waiter = nullptr;
  Project* project = nullptr;
  DiagnosticsEngine* diag_engine = nullptr;
  VFS* vfs = nullptr;
  ImportManager* import_manager = nullptr;
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

bool FindFileOrFail(DB* db,
                    Project* project,
                    std::optional<lsRequestId> id,
                    const std::string& absolute_path,
                    QueryFile** out_query_file,
                    int* out_file_id = nullptr);

void EmitInactiveLines(WorkingFile* working_file,
                       const std::vector<Range>& inactive_regions);

void EmitSemanticHighlighting(DB* db,
                              SemanticHighlightSymbolCache* semantic_cache,
                              WorkingFile* working_file,
                              QueryFile* file);
