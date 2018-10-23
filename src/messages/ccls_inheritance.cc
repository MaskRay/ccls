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

#include "hierarchy.hh"
#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

#include <unordered_set>

namespace {
MethodType kMethodType = "$ccls/inheritance",
           implementation = "textDocument/implementation";

struct In_cclsInheritance : public RequestMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    // If id+kind are specified, expand a node; otherwise textDocument+position
    // should be specified for building the root and |levels| of nodes below.
    lsTextDocumentIdentifier textDocument;
    lsPosition position;

    Usr usr;
    std::string id;
    SymbolKind kind = SymbolKind::Invalid;

    // true: derived classes/functions; false: base classes/functions
    bool derived = false;
    bool qualified = true;
    int levels = 1;
    bool hierarchy = false;
  } params;
};

MAKE_REFLECT_STRUCT(In_cclsInheritance::Params, textDocument, position, id,
                    kind, derived, qualified, levels, hierarchy);
MAKE_REFLECT_STRUCT(In_cclsInheritance, id, params);
REGISTER_IN_MESSAGE(In_cclsInheritance);

struct Out_cclsInheritance {
  Usr usr;
  std::string id;
  SymbolKind kind;
  std::string_view name;
  lsLocation location;
  // For unexpanded nodes, this is an upper bound because some entities may be
  // undefined. If it is 0, there are no members.
  int numChildren;
  // Empty if the |levels| limit is reached.
  std::vector<Out_cclsInheritance> children;
};
MAKE_REFLECT_STRUCT(Out_cclsInheritance, id, kind, name, location, numChildren,
                    children);

bool Expand(MessageHandler *m, Out_cclsInheritance *entry, bool derived,
            bool qualified, int levels);

template <typename Q>
bool ExpandHelper(MessageHandler *m, Out_cclsInheritance *entry, bool derived,
                  bool qualified, int levels, Q &entity) {
  const auto *def = entity.AnyDef();
  if (def) {
    entry->name = def->Name(qualified);
    if (def->spell) {
      if (auto loc = GetLsLocation(m->db, m->working_files, *def->spell))
        entry->location = *loc;
    } else if (entity.declarations.size()) {
      if (auto loc = GetLsLocation(m->db, m->working_files, entity.declarations[0]))
        entry->location = *loc;
    }
  } else if (!derived) {
    entry->numChildren = 0;
    return false;
  }
  std::unordered_set<Usr> seen;
  if (derived) {
    if (levels > 0) {
      for (auto usr : entity.derived) {
        if (!seen.insert(usr).second)
          continue;
        Out_cclsInheritance entry1;
        entry1.id = std::to_string(usr);
        entry1.usr = usr;
        entry1.kind = entry->kind;
        if (Expand(m, &entry1, derived, qualified, levels - 1))
          entry->children.push_back(std::move(entry1));
      }
      entry->numChildren = int(entry->children.size());
    } else
      entry->numChildren = int(entity.derived.size());
  } else {
    if (levels > 0) {
      for (auto usr : def->bases) {
        if (!seen.insert(usr).second)
          continue;
        Out_cclsInheritance entry1;
        entry1.id = std::to_string(usr);
        entry1.usr = usr;
        entry1.kind = entry->kind;
        if (Expand(m, &entry1, derived, qualified, levels - 1))
          entry->children.push_back(std::move(entry1));
      }
      entry->numChildren = int(entry->children.size());
    } else
      entry->numChildren = int(def->bases.size());
  }
  return true;
}

bool Expand(MessageHandler *m, Out_cclsInheritance *entry, bool derived,
            bool qualified, int levels) {
  if (entry->kind == SymbolKind::Func)
    return ExpandHelper(m, entry, derived, qualified, levels,
                        m->db->Func(entry->usr));
  else
    return ExpandHelper(m, entry, derived, qualified, levels,
                        m->db->Type(entry->usr));
}

struct Handler_cclsInheritance : BaseMessageHandler<In_cclsInheritance> {
  MethodType GetMethodType() const override { return kMethodType; }

  std::optional<Out_cclsInheritance> BuildInitial(SymbolRef sym, bool derived,
                                                  bool qualified, int levels) {
    Out_cclsInheritance entry;
    entry.id = std::to_string(sym.usr);
    entry.usr = sym.usr;
    entry.kind = sym.kind;
    Expand(this, &entry, derived, qualified, levels);
    return entry;
  }

  void Run(In_cclsInheritance *request) override {
    auto &params = request->params;
    std::optional<Out_cclsInheritance> result;
    if (params.id.size()) {
      try {
        params.usr = std::stoull(params.id);
      } catch (...) {
        return;
      }
      result.emplace();
      result->id = std::to_string(params.usr);
      result->usr = params.usr;
      result->kind = params.kind;
      if (!(((params.kind == SymbolKind::Func && db->HasFunc(params.usr)) ||
             (params.kind == SymbolKind::Type && db->HasType(params.usr))) &&
            Expand(this, &*result, params.derived, params.qualified,
                   params.levels)))
        result.reset();
    } else {
      QueryFile *file;
      if (!FindFileOrFail(db, project, request->id,
                          params.textDocument.uri.GetPath(), &file))
        return;
      WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);

      for (SymbolRef sym : FindSymbolsAtLocation(wfile, file, params.position))
        if (sym.kind == SymbolKind::Func || sym.kind == SymbolKind::Type) {
          result = BuildInitial(sym, params.derived, params.qualified,
                                params.levels);
          break;
        }
    }

    if (params.hierarchy)
      pipeline::Reply(request->id, result);
    else {
      auto out = FlattenHierarchy(result);
      pipeline::Reply(request->id, out);
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_cclsInheritance);

struct In_textDocumentImplementation : public RequestMessage {
  MethodType GetMethodType() const override { return implementation; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_textDocumentImplementation, id, params);
REGISTER_IN_MESSAGE(In_textDocumentImplementation);

struct Handler_textDocumentImplementation
    : BaseMessageHandler<In_textDocumentImplementation> {
  MethodType GetMethodType() const override { return implementation; }
  void Run(In_textDocumentImplementation *request) override {
    Handler_cclsInheritance handler;
    handler.db = db;
    handler.project = project;
    handler.working_files = working_files;

    In_cclsInheritance request1;
    request1.id = request->id;
    request1.params.textDocument = request->params.textDocument;
    request1.params.position = request->params.position;
    handler.Run(&request1);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_textDocumentImplementation);
} // namespace
