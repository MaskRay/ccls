// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "hierarchy.hh"
#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"

#include <unordered_set>

using namespace ccls;

namespace {

MethodType kMethodType = "$ccls/call";

enum class CallType : uint8_t {
  Direct = 0,
  Base = 1,
  Derived = 2,
  All = 1 | 2
};
MAKE_REFLECT_TYPE_PROXY(CallType);

bool operator&(CallType lhs, CallType rhs) {
  return uint8_t(lhs) & uint8_t(rhs);
}

struct In_cclsCall : public RequestMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  struct Params {
    // If id is specified, expand a node; otherwise textDocument+position should
    // be specified for building the root and |levels| of nodes below.
    lsTextDocumentIdentifier textDocument;
    lsPosition position;

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
  Params params;
};
MAKE_REFLECT_STRUCT(In_cclsCall::Params, textDocument, position, id, callee,
                    callType, qualified, levels, hierarchy);
MAKE_REFLECT_STRUCT(In_cclsCall, id, params);
REGISTER_IN_MESSAGE(In_cclsCall);

struct Out_cclsCall {
  Usr usr;
  std::string id;
  std::string_view name;
  lsLocation location;
  CallType callType = CallType::Direct;
  int numChildren;
  // Empty if the |levels| limit is reached.
  std::vector<Out_cclsCall> children;
  bool operator==(const Out_cclsCall &o) const {
    return location == o.location;
  }
  bool operator<(const Out_cclsCall &o) const { return location < o.location; }
};
MAKE_REFLECT_STRUCT(Out_cclsCall, id, name, location, callType, numChildren,
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
      if (auto loc = GetLsLocation(m->db, m->working_files,
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
          if (sym.kind == SymbolKind::Func)
            handle(sym, def->file_id, call_type);
    } else {
      for (Use use : func.uses) {
        const QueryFile &file1 = m->db->files[use.file_id];
        Maybe<ExtentRef> best;
        for (auto [sym, refcnt] : file1.symbol2refcnt)
          if (refcnt > 0 && sym.extent.Valid() &&
              sym.kind == SymbolKind::Func &&
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

struct Handler_cclsCall : BaseMessageHandler<In_cclsCall> {
  MethodType GetMethodType() const override { return kMethodType; }

  std::optional<Out_cclsCall> BuildInitial(Usr root_usr, bool callee,
                                           CallType call_type, bool qualified,
                                           int levels) {
    const auto *def = db->Func(root_usr).AnyDef();
    if (!def)
      return {};

    Out_cclsCall entry;
    entry.id = std::to_string(root_usr);
    entry.usr = root_usr;
    entry.callType = CallType::Direct;
    if (def->spell) {
      if (std::optional<lsLocation> loc =
              GetLsLocation(db, working_files, *def->spell))
        entry.location = *loc;
    }
    Expand(this, &entry, callee, call_type, qualified, levels);
    return entry;
  }

  void Run(In_cclsCall *request) override {
    auto &params = request->params;
    std::optional<Out_cclsCall> result;
    if (params.id.size()) {
      try {
        params.usr = std::stoull(params.id);
      } catch (...) {
        return;
      }
      result.emplace();
      result->id = std::to_string(params.usr);
      result->usr = params.usr;
      result->callType = CallType::Direct;
      if (db->HasFunc(params.usr))
        Expand(this, &*result, params.callee, params.callType, params.qualified,
               params.levels);
    } else {
      QueryFile *file;
      if (!FindFileOrFail(db, project, request->id,
                          params.textDocument.uri.GetPath(), &file))
        return;
      WorkingFile *working_file =
          working_files->GetFileByFilename(file->def->path);
      for (SymbolRef sym :
           FindSymbolsAtLocation(working_file, file, params.position)) {
        if (sym.kind == SymbolKind::Func) {
          result = BuildInitial(sym.usr, params.callee, params.callType,
                                params.qualified, params.levels);
          break;
        }
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
REGISTER_MESSAGE_HANDLER(Handler_cclsCall);

} // namespace
