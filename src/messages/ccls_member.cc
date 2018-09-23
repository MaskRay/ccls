// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "hierarchy.hh"
#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"

#include <clang/AST/Type.h>
#include <llvm/ADT/DenseSet.h>

#include <unordered_set>

using namespace ccls;
using namespace clang;

namespace {
MethodType kMethodType = "$ccls/member";

struct In_CclsMember : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  struct Params {
    // If id is specified, expand a node; otherwise textDocument+position should
    // be specified for building the root and |levels| of nodes below.
    lsTextDocumentIdentifier textDocument;
    lsPosition position;

    // Type
    Usr usr;
    std::string id;

    bool qualified = false;
    int levels = 1;
    // If SymbolKind::Func and the point is at a type, list member functions
    // instead of member variables.
    SymbolKind kind = SymbolKind::Var;
    bool hierarchy = false;
  } params;
};

MAKE_REFLECT_STRUCT(In_CclsMember::Params, textDocument, position, id,
                    qualified, levels, kind, hierarchy);
MAKE_REFLECT_STRUCT(In_CclsMember, id, params);
REGISTER_IN_MESSAGE(In_CclsMember);

struct Out_CclsMember : public lsOutMessage<Out_CclsMember> {
  struct Entry {
    Usr usr;
    std::string id;
    std::string_view name;
    std::string fieldName;
    lsLocation location;
    // For unexpanded nodes, this is an upper bound because some entities may be
    // undefined. If it is 0, there are no members.
    int numChildren = 0;
    // Empty if the |levels| limit is reached.
    std::vector<Entry> children;
  };
  lsRequestId id;
  std::optional<Entry> result;
};
MAKE_REFLECT_STRUCT(Out_CclsMember::Entry, id, name, fieldName,
                    location, numChildren, children);
MAKE_REFLECT_STRUCT_MANDATORY_OPTIONAL(Out_CclsMember, jsonrpc, id,
                                       result);

bool Expand(MessageHandler *m, Out_CclsMember::Entry *entry,
            bool qualified, int levels, SymbolKind memberKind);

// Add a field to |entry| which is a Func/Type.
void DoField(MessageHandler *m, Out_CclsMember::Entry *entry,
             const QueryVar &var, int64_t offset, bool qualified, int levels) {
  const QueryVar::Def *def1 = var.AnyDef();
  if (!def1)
    return;
  Out_CclsMember::Entry entry1;
  // With multiple inheritance, the offset is incorrect.
  if (offset >= 0) {
    if (offset / 8 < 10)
      entry1.fieldName += ' ';
    entry1.fieldName += std::to_string(offset / 8);
    if (offset % 8) {
      entry1.fieldName += '.';
      entry1.fieldName += std::to_string(offset % 8);
    }
    entry1.fieldName += ' ';
  }
  if (qualified)
    entry1.fieldName += def1->detailed_name;
  else {
    entry1.fieldName +=
        std::string_view(def1->detailed_name).substr(0, def1->qual_name_offset);
    entry1.fieldName += def1->Name(false);
  }
  if (def1->spell) {
    if (std::optional<lsLocation> loc =
            GetLsLocation(m->db, m->working_files, *def1->spell))
      entry1.location = *loc;
  }
  if (def1->type) {
    entry1.id = std::to_string(def1->type);
    entry1.usr = def1->type;
    if (Expand(m, &entry1, qualified, levels, SymbolKind::Var))
      entry->children.push_back(std::move(entry1));
  } else {
    entry1.id = "0";
    entry1.usr = 0;
    entry->children.push_back(std::move(entry1));
  }
}

// Expand a type node by adding members recursively to it.
bool Expand(MessageHandler *m, Out_CclsMember::Entry *entry,
            bool qualified, int levels, SymbolKind memberKind) {
  if (0 < entry->usr && entry->usr <= BuiltinType::LastKind) {
    entry->name = ClangBuiltinTypeName(int(entry->usr));
    return true;
  }
  const QueryType *type = &m->db->Type(entry->usr);
  const QueryType::Def *def = type->AnyDef();
  // builtin types have no declaration and empty |qualified|.
  if (!def)
    return false;
  entry->name = def->Name(qualified);
  std::unordered_set<Usr> seen;
  if (levels > 0) {
    std::vector<const QueryType *> stack;
    seen.insert(type->usr);
    stack.push_back(type);
    while (stack.size()) {
      type = stack.back();
      stack.pop_back();
      const auto *def = type->AnyDef();
      if (!def) continue;
      for (Usr usr : def->bases) {
        auto &type1 = m->db->Type(usr);
        if (type1.def.size()) {
          seen.insert(type1.usr);
          stack.push_back(&type1);
        }
      }
      if (def->alias_of) {
        const QueryType::Def *def1 = m->db->Type(def->alias_of).AnyDef();
        Out_CclsMember::Entry entry1;
        entry1.id = std::to_string(def->alias_of);
        entry1.usr = def->alias_of;
        if (def1 && def1->spell) {
          // The declaration of target type.
          if (std::optional<lsLocation> loc =
            GetLsLocation(m->db, m->working_files, *def1->spell))
            entry1.location = *loc;
        } else if (def->spell) {
          // Builtin types have no declaration but the typedef declaration
          // itself is useful.
          if (std::optional<lsLocation> loc =
            GetLsLocation(m->db, m->working_files, *def->spell))
            entry1.location = *loc;
        }
        if (def1 && qualified)
          entry1.fieldName = def1->detailed_name;
        if (Expand(m, &entry1, qualified, levels - 1, memberKind)) {
          // For builtin types |name| is set.
          if (entry1.fieldName.empty())
            entry1.fieldName = std::string(entry1.name);
          entry->children.push_back(std::move(entry1));
        }
      } else if (memberKind == SymbolKind::Func) {
        llvm::DenseSet<Usr, DenseMapInfoForUsr> seen1;
        for (auto &def : type->def)
          for (Usr usr : def.funcs)
            if (seen1.insert(usr).second) {
              QueryFunc &func1 = m->db->Func(usr);
              if (const QueryFunc::Def *def1 = func1.AnyDef()) {
                Out_CclsMember::Entry entry1;
                entry1.fieldName = def1->Name(false);
                if (def1->spell) {
                  if (auto loc =
                          GetLsLocation(m->db, m->working_files, *def1->spell))
                    entry1.location = *loc;
                } else if (func1.declarations.size()) {
                  if (auto loc = GetLsLocation(m->db, m->working_files,
                                               func1.declarations[0]))
                    entry1.location = *loc;
                }
                entry->children.push_back(std::move(entry1));
              }
            }
      } else if (memberKind == SymbolKind::Type) {
        llvm::DenseSet<Usr, DenseMapInfoForUsr> seen1;
        for (auto &def : type->def)
          for (Usr usr : def.types)
            if (seen1.insert(usr).second) {
              QueryType &type1 = m->db->Type(usr);
              if (const QueryType::Def *def1 = type1.AnyDef()) {
                Out_CclsMember::Entry entry1;
                entry1.fieldName = def1->Name(false);
                if (def1->spell) {
                  if (auto loc =
                    GetLsLocation(m->db, m->working_files, *def1->spell))
                    entry1.location = *loc;
                } else if (type1.declarations.size()) {
                  if (auto loc = GetLsLocation(m->db, m->working_files,
                      type1.declarations[0]))
                    entry1.location = *loc;
                }
                entry->children.push_back(std::move(entry1));
              }
            }
      } else {
        llvm::DenseSet<Usr, DenseMapInfoForUsr> seen1;
        for (auto &def : type->def)
          for (auto it : def.vars)
            if (seen1.insert(it.first).second) {
              QueryVar &var = m->db->Var(it.first);
              if (!var.def.empty())
                DoField(m, entry, var, it.second, qualified, levels - 1);
            }
      }
    }
    entry->numChildren = int(entry->children.size());
  } else
    entry->numChildren = def->alias_of ? 1 : int(def->vars.size());
  return true;
}

struct Handler_CclsMember
    : BaseMessageHandler<In_CclsMember> {
  MethodType GetMethodType() const override { return kMethodType; }

  std::optional<Out_CclsMember::Entry>
  BuildInitial(SymbolKind kind, Usr root_usr, bool qualified, int levels, SymbolKind memberKind) {
    switch (kind) {
    default:
      return {};
    case SymbolKind::Func: {
      const auto *def = db->Func(root_usr).AnyDef();
      if (!def)
        return {};

      Out_CclsMember::Entry entry;
      // Not type, |id| is invalid.
      entry.name = def->Name(qualified);
      if (def->spell) {
        if (std::optional<lsLocation> loc =
                GetLsLocation(db, working_files, *def->spell))
          entry.location = *loc;
      }
      for (Usr usr : def->vars) {
        auto &var = db->Var(usr);
        if (var.def.size())
          DoField(this, &entry, var, -1, qualified, levels - 1);
      }
      return entry;
    }
    case SymbolKind::Type: {
      const auto *def = db->Type(root_usr).AnyDef();
      if (!def)
        return {};

      Out_CclsMember::Entry entry;
      entry.id = std::to_string(root_usr);
      entry.usr = root_usr;
      if (def->spell) {
        if (std::optional<lsLocation> loc =
                GetLsLocation(db, working_files, *def->spell))
          entry.location = *loc;
      }
      Expand(this, &entry, qualified, levels, memberKind);
      return entry;
    }
    }
  }

  void Run(In_CclsMember *request) override {
    auto &params = request->params;
    Out_CclsMember out;
    out.id = request->id;

    if (params.id.size()) {
      try {
        params.usr = std::stoull(params.id);
      } catch (...) {
        return;
      }
      Out_CclsMember::Entry entry;
      entry.id = std::to_string(params.usr);
      entry.usr = params.usr;
      // entry.name is empty as it is known by the client.
      if (db->HasType(entry.usr) &&
          Expand(this, &entry, params.qualified, params.levels, params.kind))
        out.result = std::move(entry);
    } else {
      QueryFile *file;
      if (!FindFileOrFail(db, project, request->id,
                          params.textDocument.uri.GetPath(), &file))
        return;
      WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);
      for (SymbolRef sym :
           FindSymbolsAtLocation(wfile, file, params.position)) {
        switch (sym.kind) {
        case SymbolKind::Func:
        case SymbolKind::Type:
          out.result = BuildInitial(sym.kind, sym.usr, params.qualified,
                                    params.levels, params.kind);
          break;
        case SymbolKind::Var: {
          const QueryVar::Def *def = db->GetVar(sym).AnyDef();
          if (def && def->type)
            out.result = BuildInitial(SymbolKind::Type, def->type,
                                      params.qualified, params.levels, params.kind);
          break;
        }
        default:
          continue;
        }
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
      FlattenHierarchy<Out_CclsMember::Entry>(*out.result, out1);
    pipeline::WriteStdout(kMethodType, out1);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsMember);

} // namespace
