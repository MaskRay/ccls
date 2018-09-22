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

#pragma once

#include "lru_cache.h"
#include "lsp.h"
#include "method.h"
#include "query.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

struct CompletionManager;
struct Config;
struct GroupMatch;
struct VFS;
struct IncludeComplete;
struct MultiQueueWaiter;
struct Project;
struct DB;
struct WorkingFile;
struct WorkingFiles;

// Caches symbols for a single file for semantic highlighting to provide
// relatively stable ids. Only supports xxx files at a time.
struct SemanticHighlight {
  llvm::DenseMap<Usr, int, DenseMapInfoForUsr> func2id, type2id, var2id;
  uint32_t next_id = 0;
  std::unique_ptr<GroupMatch> match_;

  void Init();
  int GetStableId(SymbolKind kind, Usr usr);
};

struct Out_CclsPublishSemanticHighlighting
    : public lsOutMessage<Out_CclsPublishSemanticHighlighting> {
  struct Symbol {
    int stableId = 0;
    lsSymbolKind parentKind;
    lsSymbolKind kind;
    uint8_t storage;
    std::vector<std::pair<int, int>> ranges;

    // `lsRanges` is used to compute `ranges`.
    std::vector<lsRange> lsRanges;
  };
  struct Params {
    lsDocumentUri uri;
    std::vector<Symbol> symbols;
  };
  std::string method = "$ccls/publishSemanticHighlighting";
  Params params;
};
MAKE_REFLECT_STRUCT(Out_CclsPublishSemanticHighlighting::Symbol, stableId,
                    parentKind, kind, storage, ranges, lsRanges);
MAKE_REFLECT_STRUCT(Out_CclsPublishSemanticHighlighting::Params, uri, symbols);
MAKE_REFLECT_STRUCT(Out_CclsPublishSemanticHighlighting, jsonrpc, method,
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

#define REGISTER_MESSAGE_HANDLER(type)                                         \
  static type type##message_handler_instance_;

struct MessageHandler {
  DB *db = nullptr;
  MultiQueueWaiter *waiter = nullptr;
  Project *project = nullptr;
  VFS *vfs = nullptr;
  SemanticHighlight *highlight = nullptr;
  WorkingFiles *working_files = nullptr;
  CompletionManager *clang_complete = nullptr;
  IncludeComplete *include_complete = nullptr;

  virtual MethodType GetMethodType() const = 0;
  virtual void Run(std::unique_ptr<InMessage> message) = 0;

  static std::vector<MessageHandler *> *message_handlers;

protected:
  MessageHandler();
};

template <typename TMessage> struct BaseMessageHandler : MessageHandler {
  virtual void Run(TMessage *message) = 0;

  // MessageHandler:
  void Run(std::unique_ptr<InMessage> message) override {
    Run(static_cast<TMessage *>(message.get()));
  }
};

bool FindFileOrFail(DB *db, Project *project, std::optional<lsRequestId> id,
                    const std::string &absolute_path,
                    QueryFile **out_query_file, int *out_file_id = nullptr);

void EmitSkippedRanges(WorkingFile *working_file,
                       const std::vector<Range> &skipped_ranges);

void EmitSemanticHighlighting(DB *db, SemanticHighlight *highlight,
                              WorkingFile *working_file, QueryFile *file);
