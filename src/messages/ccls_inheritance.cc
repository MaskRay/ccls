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
#include "message_handler.hh"
#include "pipeline.hh"
#include "query.hh"

#include <unordered_set>

namespace ccls {
namespace {
struct Param : TextDocumentPositionParam {
  // If id+kind are specified, expand a node; otherwise textDocument+position
  // should be specified for building the root and |levels| of nodes below.
  Usr usr;
  std::string id;
  Kind kind = Kind::Invalid;

  // true: derived classes/functions; false: base classes/functions
  bool derived = false;
  bool qualified = true;
  int levels = 1;
  bool hierarchy = false;
};

REFLECT_STRUCT(Param, textDocument, position, id, kind, derived, qualified,
               levels, hierarchy);

struct Out_cclsInheritance {
  Usr usr;
  std::string id;
  Kind kind;
  std::string_view name;
  Location location;
  // For unexpanded nodes, this is an upper bound because some entities may be
  // undefined. If it is 0, there are no members.
  int numChildren;
  // Empty if the |levels| limit is reached.
  std::vector<Out_cclsInheritance> children;
};
REFLECT_STRUCT(Out_cclsInheritance, id, kind, name, location, numChildren,
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
      if (auto loc = GetLsLocation(m->db, m->wfiles, *def->spell))
        entry->location = *loc;
    } else if (entity.declarations.size()) {
      if (auto loc = GetLsLocation(m->db, m->wfiles, entity.declarations[0]))
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
  if (entry->kind == Kind::Func)
    return ExpandHelper(m, entry, derived, qualified, levels,
                        m->db->Func(entry->usr));
  else
    return ExpandHelper(m, entry, derived, qualified, levels,
                        m->db->Type(entry->usr));
}

std::optional<Out_cclsInheritance> BuildInitial(MessageHandler *m, SymbolRef sym, bool derived,
                                                bool qualified, int levels) {
  Out_cclsInheritance entry;
  entry.id = std::to_string(sym.usr);
  entry.usr = sym.usr;
  entry.kind = sym.kind;
  Expand(m, &entry, derived, qualified, levels);
  return entry;
}

void Inheritance(MessageHandler *m, Param &param, ReplyOnce &reply) {
  std::optional<Out_cclsInheritance> result;
  if (param.id.size()) {
    try {
      param.usr = std::stoull(param.id);
    } catch (...) {
      return;
    }
    result.emplace();
    result->id = std::to_string(param.usr);
    result->usr = param.usr;
    result->kind = param.kind;
    if (!(((param.kind == Kind::Func && m->db->HasFunc(param.usr)) ||
           (param.kind == Kind::Type && m->db->HasType(param.usr))) &&
          Expand(m, &*result, param.derived, param.qualified, param.levels)))
      result.reset();
  } else {
    auto [file, wf] = m->FindOrFail(param.textDocument.uri.GetPath(), reply);
    if (!wf) {
      return;
    }
    for (SymbolRef sym : FindSymbolsAtLocation(wf, file, param.position))
      if (sym.kind == Kind::Func || sym.kind == Kind::Type) {
        result = BuildInitial(m, sym, param.derived, param.qualified,
                              param.levels);
        break;
      }
  }

  if (param.hierarchy)
    reply(result);
  else
    reply(FlattenHierarchy(result));
}
} // namespace

void MessageHandler::ccls_inheritance(JsonReader &reader, ReplyOnce &reply) {
  Param param;
  Reflect(reader, param);
  Inheritance(this, param, reply);
}

void MessageHandler::textDocument_implementation(
    TextDocumentPositionParam &param, ReplyOnce &reply) {
  Param param1;
  param1.textDocument = param.textDocument;
  param1.position = param.position;
  param1.derived = true;
  Inheritance(this, param1, reply);
}
} // namespace ccls
