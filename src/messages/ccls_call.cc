// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "hierarchy.hh"
#include "message_handler.hh"
#include "pipeline.hh"
#include "query.hh"

#include <map>
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

struct Out_incomingCall {
  CallHierarchyItem from;
  std::vector<lsRange> fromRanges;
};
REFLECT_STRUCT(Out_incomingCall, from, fromRanges);

struct Out_outgoingCall {
  CallHierarchyItem to;
  std::vector<lsRange> fromRanges;
};
REFLECT_STRUCT(Out_outgoingCall, to, fromRanges);

bool expand(MessageHandler *m, Out_cclsCall *entry, bool callee,
            CallType call_type, bool qualified, int levels) {
  const QueryFunc &func = m->db->getFunc(entry->usr);
  const QueryFunc::Def *def = func.anyDef();
  entry->numChildren = 0;
  if (!def)
    return false;
  auto handle = [&](SymbolRef sym, int file_id, CallType call_type1) {
    entry->numChildren++;
    if (levels > 0) {
      Out_cclsCall entry1;
      entry1.id = std::to_string(sym.usr);
      entry1.usr = sym.usr;
      if (auto loc = getLsLocation(m->db, m->wfiles,
                                   Use{{sym.range, sym.role}, file_id}))
        entry1.location = *loc;
      entry1.callType = call_type1;
      if (expand(m, &entry1, callee, call_type, qualified, levels - 1))
        entry->children.push_back(std::move(entry1));
    }
  };
  auto handle_uses = [&](const QueryFunc &func, CallType call_type) {
    if (callee) {
      if (const auto *def = func.anyDef())
        for (SymbolRef sym : def->callees)
          if (sym.kind == Kind::Func)
            handle(sym, def->file_id, call_type);
    } else {
      for (Use use : func.uses) {
        const QueryFile &file1 = m->db->files[use.file_id];
        Maybe<ExtentRef> best;
        for (auto [sym, refcnt] : file1.symbol2refcnt)
          if (refcnt > 0 && sym.extent.valid() && sym.kind == Kind::Func &&
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
  entry->name = def->name(qualified);
  handle_uses(func, CallType::Direct);

  // Callers/callees of base functions.
  if (call_type & CallType::Base) {
    stack.push_back(&func);
    while (stack.size()) {
      const QueryFunc &func1 = *stack.back();
      stack.pop_back();
      if (auto *def1 = func1.anyDef()) {
        eachDefinedFunc(m->db, def1->bases, [&](QueryFunc &func2) {
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
      eachDefinedFunc(m->db, func1.derived, [&](QueryFunc &func2) {
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

std::optional<Out_cclsCall> buildInitial(MessageHandler *m, Usr root_usr,
                                         bool callee, CallType call_type,
                                         bool qualified, int levels) {
  const auto *def = m->db->getFunc(root_usr).anyDef();
  if (!def)
    return {};

  Out_cclsCall entry;
  entry.id = std::to_string(root_usr);
  entry.usr = root_usr;
  entry.callType = CallType::Direct;
  if (def->spell) {
    if (auto loc = getLsLocation(m->db, m->wfiles, *def->spell))
      entry.location = *loc;
  }
  expand(m, &entry, callee, call_type, qualified, levels);
  return entry;
}
} // namespace

void MessageHandler::ccls_call(JsonReader &reader, ReplyOnce &reply) {
  Param param;
  reflect(reader, param);
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
    if (db->hasFunc(param.usr))
      expand(this, &*result, param.callee, param.callType, param.qualified,
             param.levels);
  } else {
    auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
    if (!wf)
      return;
    for (SymbolRef sym : findSymbolsAtLocation(wf, file, param.position)) {
      if (sym.kind == Kind::Func) {
        result = buildInitial(this, sym.usr, param.callee, param.callType,
                              param.qualified, param.levels);
        break;
      }
    }
  }

  if (param.hierarchy)
    reply(result);
  else
    reply(flattenHierarchy(result));
}

void MessageHandler::textDocument_prepareCallHierarchy(
    TextDocumentPositionParam &param, ReplyOnce &reply) {
  std::string path = param.textDocument.uri.getPath();
  auto [file, wf] = findOrFail(path, reply);
  if (!file)
    return;

  std::vector<CallHierarchyItem> result;
  for (SymbolRef sym : findSymbolsAtLocation(wf, file, param.position)) {
    if (sym.kind != Kind::Func)
      continue;
    const auto *def = db->getFunc(sym.usr).anyDef();
    if (!def)
      continue;
    auto r = getLsRange(wf, sym.range);
    if (!r)
      continue;
    CallHierarchyItem &item = result.emplace_back();
    item.name = def->name(false);
    item.kind = def->kind;
    item.detail = def->name(true);
    item.uri = DocumentUri::fromPath(path);
    item.range = item.selectionRange = *r;
    item.data = std::to_string(sym.usr);
  }
  reply(result);
}

static lsRange toLsRange(Range r) {
  return {{r.start.line, r.start.column}, {r.end.line, r.end.column}};
}

static void
add(std::map<SymbolIdx, std::pair<int, std::vector<lsRange>>> &sym2ranges,
    SymbolRef sym, int file_id) {
  auto [it, inserted] = sym2ranges.try_emplace(SymbolIdx{sym.usr, sym.kind});
  if (inserted)
    it->second.first = file_id;
  if (it->second.first == file_id)
    it->second.second.push_back(toLsRange(sym.range));
}

template <typename Out>
static std::vector<Out> toCallResult(
    DB *db,
    const std::map<SymbolIdx, std::pair<int, std::vector<lsRange>>> &sym2ranges) {
  std::vector<Out> result;
  for (auto &[sym, ranges] : sym2ranges) {
    CallHierarchyItem item;
    item.uri = getLsDocumentUri(db, ranges.first);
    auto r = ranges.second[0];
    item.range = {{uint16_t(r.start.line), int16_t(r.start.character)},
                  {uint16_t(r.end.line), int16_t(r.end.character)}};
    item.selectionRange = item.range;
    switch (sym.kind) {
    default:
      continue;
    case Kind::Func: {
      auto idx = db->func_usr[sym.usr];
      const QueryFunc &func = db->funcs[idx];
      const QueryFunc::Def *def = func.anyDef();
      if (!def)
        continue;
      item.name = def->name(false);
      item.kind = def->kind;
      item.detail = def->name(true);
      item.data = std::to_string(sym.usr);
    }
    }

    result.push_back({std::move(item), std::move(ranges.second)});
  }
  return result;
}

void MessageHandler::callHierarchy_incomingCalls(CallsParam &param,
                                                 ReplyOnce &reply) {
  Usr usr;
  try {
    usr = std::stoull(param.item.data);
  } catch (...) {
    return;
  }
  const QueryFunc &func = db->getFunc(usr);
  std::map<SymbolIdx, std::pair<int, std::vector<lsRange>>> sym2ranges;
  for (Use use : func.uses) {
    const QueryFile &file = db->files[use.file_id];
    Maybe<ExtentRef> best;
    for (auto [sym, refcnt] : file.symbol2refcnt)
      if (refcnt > 0 && sym.extent.valid() && sym.kind == Kind::Func &&
          sym.extent.start <= use.range.start &&
          use.range.end <= sym.extent.end &&
          (!best || best->extent.start < sym.extent.start))
        best = sym;
    if (best)
      add(sym2ranges, *best, use.file_id);
  }
  reply(toCallResult<Out_incomingCall>(db, sym2ranges));
}

void MessageHandler::callHierarchy_outgoingCalls(CallsParam &param,
                                                 ReplyOnce &reply) {
  Usr usr;
  try {
    usr = std::stoull(param.item.data);
  } catch (...) {
    return;
  }
  const QueryFunc &func = db->getFunc(usr);
  std::map<SymbolIdx, std::pair<int, std::vector<lsRange>>> sym2ranges;
  if (const auto *def = func.anyDef())
    for (SymbolRef sym : def->callees)
      if (sym.kind == Kind::Func) {
        add(sym2ranges, sym, def->file_id);
      }
  reply(toCallResult<Out_outgoingCall>(db, sym2ranges));
}
} // namespace ccls
