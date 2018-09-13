// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "hierarchy.hh"
#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

#include <unordered_set>

namespace {
MethodType kMethodType = "$ccls/inheritance";

struct In_CclsInheritance : public RequestInMessage {
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

MAKE_REFLECT_STRUCT(In_CclsInheritance::Params, textDocument, position,
                    id, kind, derived, qualified, levels, hierarchy);
MAKE_REFLECT_STRUCT(In_CclsInheritance, id, params);
REGISTER_IN_MESSAGE(In_CclsInheritance);

struct Out_CclsInheritance
    : public lsOutMessage<Out_CclsInheritance> {
  struct Entry {
    Usr usr;
    std::string id;
    SymbolKind kind;
    std::string_view name;
    lsLocation location;
    // For unexpanded nodes, this is an upper bound because some entities may be
    // undefined. If it is 0, there are no members.
    int numChildren;
    // Empty if the |levels| limit is reached.
    std::vector<Entry> children;
  };
  lsRequestId id;
  std::optional<Entry> result;
};
MAKE_REFLECT_STRUCT(Out_CclsInheritance::Entry, id, kind, name,
                    location, numChildren, children);
MAKE_REFLECT_STRUCT_MANDATORY_OPTIONAL(Out_CclsInheritance, jsonrpc,
                                       id, result);

bool Expand(MessageHandler *m, Out_CclsInheritance::Entry *entry,
            bool derived, bool qualified, int levels);

template <typename Q>
bool ExpandHelper(MessageHandler *m, Out_CclsInheritance::Entry *entry,
                  bool derived, bool qualified, int levels, Q &entity) {
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
        Out_CclsInheritance::Entry entry1;
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
        Out_CclsInheritance::Entry entry1;
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

bool Expand(MessageHandler *m, Out_CclsInheritance::Entry *entry,
            bool derived, bool qualified, int levels) {
  if (entry->kind == SymbolKind::Func)
    return ExpandHelper(m, entry, derived, qualified, levels,
                        m->db->Func(entry->usr));
  else
    return ExpandHelper(m, entry, derived, qualified, levels,
                        m->db->Type(entry->usr));
}

struct Handler_CclsInheritance
    : BaseMessageHandler<In_CclsInheritance> {
  MethodType GetMethodType() const override { return kMethodType; }

  std::optional<Out_CclsInheritance::Entry>
  BuildInitial(SymbolRef sym, bool derived, bool qualified, int levels) {
    Out_CclsInheritance::Entry entry;
    entry.id = std::to_string(sym.usr);
    entry.usr = sym.usr;
    entry.kind = sym.kind;
    Expand(this, &entry, derived, qualified, levels);
    return entry;
  }

  void Run(In_CclsInheritance *request) override {
    auto &params = request->params;
    Out_CclsInheritance out;
    out.id = request->id;

    if (params.id.size()) {
      try {
        params.usr = std::stoull(params.id);
      } catch (...) {
        return;
      }
      Out_CclsInheritance::Entry entry;
      entry.id = std::to_string(params.usr);
      entry.usr = params.usr;
      entry.kind = params.kind;
      if (((entry.kind == SymbolKind::Func && db->HasFunc(entry.usr)) ||
           (entry.kind == SymbolKind::Type && db->HasType(entry.usr))) &&
          Expand(this, &entry, params.derived, params.qualified, params.levels))
        out.result = std::move(entry);
    } else {
      QueryFile *file;
      if (!FindFileOrFail(db, project, request->id,
                          params.textDocument.uri.GetPath(), &file))
        return;
      WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);

      for (SymbolRef sym : FindSymbolsAtLocation(wfile, file, params.position))
        if (sym.kind == SymbolKind::Func || sym.kind == SymbolKind::Type) {
          out.result = BuildInitial(sym, params.derived, params.qualified,
                                    params.levels);
          break;
        }
    }

    if (params.hierarchy) {
      pipeline::WriteStdout(kMethodType, out);
      return;
    }
    Out_LocationList out1;
    out1.id = request->id;
    if (out.result)
      FlattenHierarchy<Out_CclsInheritance::Entry>(*out.result, out1);
    pipeline::WriteStdout(kMethodType, out1);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsInheritance);

} // namespace
