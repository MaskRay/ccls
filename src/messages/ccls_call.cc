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

enum class CallType : uint8_t {
  Direct = 0,
  Base = 1,
  Derived = 2,
  All = 1 | 2
};
REFLECT_UNDERLYING(CallType);

bool operator&(CallType lhs, CallType rhs) {
  return uint8_t(lhs) & uint8_t(rhs);
}

struct Param : TextDocumentPositionParam {
  // If id is specified, expand a node; otherwise textDocument+position should
  // be specified for building the root and |levels| of nodes below.
  Usr usr;
  std::string id;

  // true: callee tree (functions called by this function); false: caller tree
  // (where this function is called)
  bool callee = false;
  // Base: include base functions; All: include both base and derived
  // functions.
  CallType callType = CallType::All;
  bool qualified = true;
  int levels = 1;
  bool hierarchy = false;
};
REFLECT_STRUCT(Param, textDocument, position, id, callee, callType, qualified,
               levels, hierarchy);

struct Out_cclsCall {
  Usr usr;
  std::string id;
  std::string_view name;
  Location location;
  CallType callType = CallType::Direct;
  int numChildren;
  // Empty if the |levels| limit is reached.
  std::vector<Out_cclsCall> children;
  bool operator==(const Out_cclsCall &o) const {
    return location == o.location;
  }
  bool operator<(const Out_cclsCall &o) const { return location < o.location; }
};
REFLECT_STRUCT(Out_cclsCall, id, name, location, callType, numChildren,
               children);

bool Expand(MessageHandler *m, Out_cclsCall *entry, bool callee,
            CallType call_type, bool qualified, int levels) {
  const QueryFunc &func = m->db->Func(entry->usr);
  const QueryFunc::Def *def = func.AnyDef();
  entry->numChildren = 0;
  if (!def)
    return false;
  auto handle = [&](SymbolRef sym, int file_id, CallType call_type1) {
    entry->numChildren++;
    if (levels > 0) {
      Out_cclsCall entry1;
      entry1.id = std::to_string(sym.usr);
      entry1.usr = sym.usr;
      if (auto loc = GetLsLocation(m->db, m->wfiles,
                                   Use{{sym.range, sym.role}, file_id}))
        entry1.location = *loc;
      entry1.callType = call_type1;
      if (Expand(m, &entry1, callee, call_type, qualified, levels - 1))
        entry->children.push_back(std::move(entry1));
    }
  };
  auto handle_uses = [&](const QueryFunc &func, CallType call_type) {
    if (callee) {
      if (const auto *def = func.AnyDef())
        for (SymbolRef sym : def->callees)
          if (sym.kind == Kind::Func)
            handle(sym, def->file_id, call_type);
    } else {
      for (Use use : func.uses) {
        const QueryFile &file1 = m->db->files[use.file_id];
        Maybe<ExtentRef> best;
        for (auto [sym, refcnt] : file1.symbol2refcnt)
          if (refcnt > 0 && sym.extent.Valid() && sym.kind == Kind::Func &&
              sym.extent.start <= use.range.start &&
              use.range.end <= sym.extent.end &&
              (!best || best->extent.start < sym.extent.start))
            best = sym;
        if (best)
          handle(*best, use.file_id, call_type);
      }
    }
  };

  std::unordered_set<Usr> seen;
  seen.insert(func.usr);
  std::vector<const QueryFunc *> stack;
  entry->name = def->Name(qualified);
  handle_uses(func, CallType::Direct);

  // Callers/callees of base functions.
  if (call_type & CallType::Base) {
    stack.push_back(&func);
    while (stack.size()) {
      const QueryFunc &func1 = *stack.back();
      stack.pop_back();
      if (auto *def1 = func1.AnyDef()) {
        EachDefinedFunc(m->db, def1->bases, [&](QueryFunc &func2) {
          if (!seen.count(func2.usr)) {
            seen.insert(func2.usr);
            stack.push_back(&func2);
            handle_uses(func2, CallType::Base);
          }
        });
      }
    }
  }

  // Callers/callees of derived functions.
  if (call_type & CallType::Derived) {
    stack.push_back(&func);
    while (stack.size()) {
      const QueryFunc &func1 = *stack.back();
      stack.pop_back();
      EachDefinedFunc(m->db, func1.derived, [&](QueryFunc &func2) {
        if (!seen.count(func2.usr)) {
          seen.insert(func2.usr);
          stack.push_back(&func2);
          handle_uses(func2, CallType::Derived);
        }
      });
    }
  }

  std::sort(entry->children.begin(), entry->children.end());
  entry->children.erase(
      std::unique(entry->children.begin(), entry->children.end()),
      entry->children.end());
  return true;
}

std::optional<Out_cclsCall> BuildInitial(MessageHandler *m, Usr root_usr,
                                         bool callee, CallType call_type,
                                         bool qualified, int levels) {
  const auto *def = m->db->Func(root_usr).AnyDef();
  if (!def)
    return {};

  Out_cclsCall entry;
  entry.id = std::to_string(root_usr);
  entry.usr = root_usr;
  entry.callType = CallType::Direct;
  if (def->spell) {
    if (auto loc = GetLsLocation(m->db, m->wfiles, *def->spell))
      entry.location = *loc;
  }
  Expand(m, &entry, callee, call_type, qualified, levels);
  return entry;
}
} // namespace

void MessageHandler::ccls_call(JsonReader &reader, ReplyOnce &reply) {
  Param param;
  Reflect(reader, param);
  std::optional<Out_cclsCall> result;
  if (param.id.size()) {
    try {
      param.usr = std::stoull(param.id);
    } catch (...) {
      return;
    }
    result.emplace();
    result->id = std::to_string(param.usr);
    result->usr = param.usr;
    result->callType = CallType::Direct;
    if (db->HasFunc(param.usr))
      Expand(this, &*result, param.callee, param.callType, param.qualified,
             param.levels);
  } else {
    auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply);
    if (!wf)
      return;
    for (SymbolRef sym : FindSymbolsAtLocation(wf, file, param.position)) {
      if (sym.kind == Kind::Func) {
        result = BuildInitial(this, sym.usr, param.callee, param.callType,
                              param.qualified, param.levels);
        break;
      }
    }
  }

  if (param.hierarchy)
    reply(result);
  else
    reply(FlattenHierarchy(result));
}
} // namespace ccls
