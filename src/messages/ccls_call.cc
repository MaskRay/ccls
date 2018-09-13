// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "hierarchy.hh"
#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

#include <unordered_set>

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

struct In_CclsCall : public RequestInMessage {
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
MAKE_REFLECT_STRUCT(In_CclsCall::Params, textDocument, position, id,
                    callee, callType, qualified, levels, hierarchy);
MAKE_REFLECT_STRUCT(In_CclsCall, id, params);
REGISTER_IN_MESSAGE(In_CclsCall);

struct Out_CclsCall : public lsOutMessage<Out_CclsCall> {
  struct Entry {
    Usr usr;
    std::string id;
    std::string_view name;
    lsLocation location;
    CallType callType = CallType::Direct;
    int numChildren;
    // Empty if the |levels| limit is reached.
    std::vector<Entry> children;
  };

  lsRequestId id;
  std::optional<Entry> result;
};
MAKE_REFLECT_STRUCT(Out_CclsCall::Entry, id, name, location, callType,
                    numChildren, children);
MAKE_REFLECT_STRUCT_MANDATORY_OPTIONAL(Out_CclsCall, jsonrpc, id,
                                       result);

bool Expand(MessageHandler *m, Out_CclsCall::Entry *entry, bool callee,
            CallType call_type, bool qualified, int levels) {
  const QueryFunc &func = m->db->Func(entry->usr);
  const QueryFunc::Def *def = func.AnyDef();
  entry->numChildren = 0;
  if (!def)
    return false;
  auto handle = [&](Use use, CallType call_type1) {
    entry->numChildren++;
    if (levels > 0) {
      Out_CclsCall::Entry entry1;
      entry1.id = std::to_string(use.usr);
      entry1.usr = use.usr;
      if (auto loc = GetLsLocation(m->db, m->working_files, use))
        entry1.location = *loc;
      entry1.callType = call_type1;
      if (Expand(m, &entry1, callee, call_type, qualified, levels - 1))
        entry->children.push_back(std::move(entry1));
    }
  };
  auto handle_uses = [&](const QueryFunc &func, CallType call_type) {
    if (callee) {
      if (const auto *def = func.AnyDef())
        for (SymbolRef ref : def->callees)
          if (ref.kind == SymbolKind::Func)
            handle(Use{{ref.range, ref.usr, ref.kind, ref.role}, def->file_id},
                   call_type);
    } else {
      for (Use use : func.uses)
        if (use.kind == SymbolKind::Func)
          handle(use, call_type);
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
  return true;
}

struct Handler_CclsCall : BaseMessageHandler<In_CclsCall> {
  MethodType GetMethodType() const override { return kMethodType; }

  std::optional<Out_CclsCall::Entry>
  BuildInitial(Usr root_usr, bool callee, CallType call_type, bool qualified,
               int levels) {
    const auto *def = db->Func(root_usr).AnyDef();
    if (!def)
      return {};

    Out_CclsCall::Entry entry;
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

  void Run(In_CclsCall *request) override {
    auto &params = request->params;
    Out_CclsCall out;
    out.id = request->id;

    if (params.id.size()) {
      try {
        params.usr = std::stoull(params.id);
      } catch (...) {
        return;
      }
      Out_CclsCall::Entry entry;
      entry.id = std::to_string(params.usr);
      entry.usr = params.usr;
      entry.callType = CallType::Direct;
      if (db->HasFunc(params.usr))
        Expand(this, &entry, params.callee, params.callType, params.qualified,
               params.levels);
      out.result = std::move(entry);
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
          out.result = BuildInitial(sym.usr, params.callee, params.callType,
                                    params.qualified, params.levels);
          break;
        }
      }
    }

    if (params.hierarchy) {
      pipeline::WriteStdout(kMethodType, out);
      return;
    }
    Out_LocationList out1;
    out1.id = request->id;
    if (out.result)
      FlattenHierarchy<Out_CclsCall::Entry>(*out.result, out1);
    pipeline::WriteStdout(kMethodType, out1);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsCall);

} // namespace
