#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

#include <loguru.hpp>

namespace {
struct Ipc_CqueryCallHierarchyInitial
    : public RequestMessage<Ipc_CqueryCallHierarchyInitial> {
  const static IpcId kIpcId = IpcId::CqueryCallHierarchyInitial;
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    bool callee = false;
    int levels = 1;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryCallHierarchyInitial::Params,
                    textDocument,
                    position,
                    callee,
                    levels);
MAKE_REFLECT_STRUCT(Ipc_CqueryCallHierarchyInitial, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallHierarchyInitial);

enum class CallType { Direct = 0, Base = 1, Derived = 2, All = 1 | 2 };
MAKE_REFLECT_TYPE_PROXY(CallType);

struct Ipc_CqueryCallHierarchyExpand
    : public RequestMessage<Ipc_CqueryCallHierarchyExpand> {
  const static IpcId kIpcId = IpcId::CqueryCallHierarchyExpand;
  struct Params {
    Maybe<QueryFuncId> id;
    // true: callee tree; false: caller tree
    bool callee = false;
    int levels = 1;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryCallHierarchyExpand::Params, id, callee, levels);
MAKE_REFLECT_STRUCT(Ipc_CqueryCallHierarchyExpand, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallHierarchyExpand);

struct Out_CqueryCallHierarchy : public lsOutMessage<Out_CqueryCallHierarchy> {
  struct Entry {
    QueryFuncId id;
    std::string_view name;
    lsLocation location;
    CallType callType = CallType::Direct;
    int numChildren;
    // Empty if the |levels| limit is reached.
    std::vector<Entry> children;
  };

  lsRequestId id;
  optional<Entry> result;
};
MAKE_REFLECT_STRUCT(Out_CqueryCallHierarchy::Entry,
                    id,
                    name,
                    location,
                    callType,
                    numChildren,
                    children);
MAKE_REFLECT_STRUCT(Out_CqueryCallHierarchy, jsonrpc, id, result);

void Expand(MessageHandler* m,
            Out_CqueryCallHierarchy::Entry* entry,
            bool callee,
            int levels) {
  const QueryFunc& func = m->db->funcs[entry->id.id];
  const QueryFunc::Def* def = func.AnyDef();
  entry->numChildren = 0;
  if (!def)
    return;
  if (def->spell) {
    if (optional<lsLocation> loc =
        GetLsLocation(m->db, m->working_files, *def->spell))
      entry->location = *loc;
  }
  auto handle = [&](Use use, CallType call_type) {
    QueryFunc& rel_func = m->db->GetFunc(use);
    const QueryFunc::Def* rel_def = rel_func.AnyDef();
    if (!rel_def)
      return;
    if (optional<lsLocation> loc =
            GetLsLocation(m->db, m->working_files, use)) {
      entry->numChildren++;
      if (levels > 0) {
        Out_CqueryCallHierarchy::Entry entry1;
        entry1.id = QueryFuncId(use.id);
        entry1.name = rel_def->ShortName();
        entry1.location = *loc;
        entry1.callType = call_type;
        Expand(m, &entry1, callee, levels - 1);
        entry->children.push_back(std::move(entry1));
      }
    }
  };
  auto handle_uses = [&](const QueryFunc& func, CallType call_type) {
    if (callee) {
      if (const auto* def = func.AnyDef())
        for (SymbolRef ref : def->callees)
          if (ref.kind == SymbolKind::Func)
            handle(Use(ref.range, ref.id, ref.kind, ref.role, def->file), call_type);
    } else {
      for (Use use : func.uses)
        if (use.kind == SymbolKind::Func)
          handle(use, call_type);
    }
  };

  std::unordered_set<Usr> seen;
  std::vector<const QueryFunc*> stack;
  handle_uses(func, CallType::Direct);

  // Callers/callees of base functions.
  stack.push_back(&func);
  while (stack.size()) {
    const QueryFunc& func1 = *stack.back();
    stack.pop_back();
    if (auto* def1 = func1.AnyDef()) {
      EachDefinedEntity(m->db->funcs, def1->base, [&](QueryFunc& func2) {
        if (seen.count(func2.usr)) {
          seen.insert(func2.usr);
          stack.push_back(&func2);
          handle_uses(func2, CallType::Base);
        }
      });
    }
  }

  // Callers/callees of derived functions.
  stack.push_back(&func);
  while (stack.size()) {
    const QueryFunc& func1 = *stack.back();
    stack.pop_back();
    EachDefinedEntity(m->db->funcs, func1.derived, [&](QueryFunc& func2) {
      if (seen.count(func2.usr)) {
        seen.insert(func2.usr);
        stack.push_back(&func2);
        handle_uses(func2, CallType::Derived);
      }
    });
  }
}

struct CqueryCallHierarchyInitialHandler
    : BaseMessageHandler<Ipc_CqueryCallHierarchyInitial> {
  optional<Out_CqueryCallHierarchy::Entry> BuildInitial(QueryFuncId root_id,
                                                        bool callee,
                                                        int levels) {
    const auto* def = db->funcs[root_id.id].AnyDef();
    if (!def)
      return {};

    Out_CqueryCallHierarchy::Entry entry;
    entry.id = root_id;
    entry.name = def->ShortName();
    Expand(this, &entry, callee, levels);
    return entry;
  }

  void Run(Ipc_CqueryCallHierarchyInitial* request) override {
    QueryFile* file;
    const auto& params = request->params;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);
    Out_CqueryCallHierarchy out;
    out.id = request->id;

    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, params.position)) {
      if (sym.kind == SymbolKind::Func) {
        out.result =
            BuildInitial(QueryFuncId(sym.id), params.callee, params.levels);
        break;
      }
    }

    QueueManager::WriteStdout(IpcId::CqueryCallHierarchyInitial, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallHierarchyInitialHandler);

struct CqueryCallHierarchyExpandHandler
    : BaseMessageHandler<Ipc_CqueryCallHierarchyExpand> {
  void Run(Ipc_CqueryCallHierarchyExpand* request) override {
    const auto& params = request->params;
    Out_CqueryCallHierarchy out;
    out.id = request->id;
    if (params.id) {
      Out_CqueryCallHierarchy::Entry entry;
      entry.id = *params.id;
      // entry.name is empty and it is known by the client.
      if (entry.id.id < db->funcs.size())
        Expand(this, &entry, params.callee, params.levels);
      out.result = std::move(entry);
    }

    QueueManager::WriteStdout(IpcId::CqueryCallHierarchyExpand, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallHierarchyExpandHandler);
}  // namespace
