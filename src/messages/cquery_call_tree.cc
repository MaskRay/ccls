#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

#include <loguru.hpp>

// FIXME Interop with VSCode, change std::string usr to Usr (uint64_t)
namespace {
struct Ipc_CqueryCallTreeInitial
    : public RequestMessage<Ipc_CqueryCallTreeInitial> {
  const static IpcId kIpcId = IpcId::CqueryCallTreeInitial;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryCallTreeInitial, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallTreeInitial);

struct Ipc_CqueryCallTreeExpand
    : public RequestMessage<Ipc_CqueryCallTreeExpand> {
  const static IpcId kIpcId = IpcId::CqueryCallTreeExpand;
  struct Params {
    std::string usr;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryCallTreeExpand::Params, usr);
MAKE_REFLECT_STRUCT(Ipc_CqueryCallTreeExpand, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallTreeExpand);

struct Out_CqueryCallTree : public lsOutMessage<Out_CqueryCallTree> {
  enum class CallType { Direct = 0, Base = 1, Derived = 2 };
  struct CallEntry {
    std::string_view name;
    std::string usr;
    lsLocation location;
    bool hasCallers = true;
    CallType callType = CallType::Direct;
  };

  lsRequestId id;
  std::vector<CallEntry> result;
};
MAKE_REFLECT_TYPE_PROXY(Out_CqueryCallTree::CallType);
MAKE_REFLECT_STRUCT(Out_CqueryCallTree::CallEntry,
                    name,
                    usr,
                    location,
                    hasCallers,
                    callType);
MAKE_REFLECT_STRUCT(Out_CqueryCallTree, jsonrpc, id, result);

std::vector<Out_CqueryCallTree::CallEntry> BuildInitialCallTree(
    QueryDatabase* db,
    WorkingFiles* working_files,
    QueryFuncId root) {
  QueryFunc& root_func = db->funcs[root.id];
  if (!root_func.def || !root_func.def->definition_spelling)
    return {};
  optional<lsLocation> def_loc =
      GetLsLocation(db, working_files, *root_func.def->definition_spelling);
  if (!def_loc)
    return {};

  Out_CqueryCallTree::CallEntry entry;
  entry.name = root_func.def->ShortName();
  entry.usr = std::to_string(root_func.usr);
  entry.location = *def_loc;
  entry.hasCallers = HasCallersOnSelfOrBaseOrDerived(db, root_func);
  std::vector<Out_CqueryCallTree::CallEntry> result;
  result.push_back(entry);
  return result;
}

std::vector<Out_CqueryCallTree::CallEntry> BuildExpandCallTree(
    QueryDatabase* db,
    WorkingFiles* working_files,
    QueryFuncId root) {
  QueryFunc& root_func = db->funcs[root.id];
  if (!root_func.def)
    return {};

  std::vector<Out_CqueryCallTree::CallEntry> result;

  auto handle_caller = [&](QueryFuncRef caller,
                           Out_CqueryCallTree::CallType call_type) {
    optional<lsLocation> call_location =
        GetLsLocation(db, working_files, caller);
    if (!call_location)
      return;

    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: basically, querydb gets duplicate references inserted into it.
    // if (!seen_locations.insert(caller.loc).second) {
    //  LOG_S(ERROR) << "!!!! FIXME DUPLICATE REFERENCE IN QUERYDB" <<
    //  std::endl; return;
    //}

    if (caller.kind == SymbolKind::Func) {
      QueryFunc& call_func = db->GetFunc(caller);
      if (!call_func.def)
        return;

      Out_CqueryCallTree::CallEntry call_entry;
      call_entry.name = call_func.def->ShortName();
      call_entry.usr = std::to_string(call_func.usr);
      call_entry.location = *call_location;
      call_entry.hasCallers = HasCallersOnSelfOrBaseOrDerived(db, call_func);
      call_entry.callType = call_type;
      result.push_back(call_entry);
    } else {
      // TODO: See if we can do a better job here. Need more information from
      // the indexer.
      Out_CqueryCallTree::CallEntry call_entry;
      call_entry.name = "Likely Constructor";
      call_entry.usr = "no_usr";
      call_entry.location = *call_location;
      call_entry.hasCallers = false;
      call_entry.callType = call_type;
      result.push_back(call_entry);
    }
  };

  std::vector<QueryFuncRef> base_callers =
      GetCallersForAllBaseFunctions(db, root_func);
  std::vector<QueryFuncRef> derived_callers =
      GetCallersForAllDerivedFunctions(db, root_func);
  result.reserve(root_func.uses.size() + base_callers.size() +
                 derived_callers.size());

  for (QueryFuncRef caller : root_func.uses)
    handle_caller(caller, Out_CqueryCallTree::CallType::Direct);
  for (QueryFuncRef caller : base_callers)
    if (caller.kind == SymbolKind::Func && caller.id != Id<void>(root)) {
      // Do not show calls to the base function coming from this function.
      handle_caller(caller, Out_CqueryCallTree::CallType::Base);
    }
  for (QueryFuncRef caller : derived_callers)
    handle_caller(caller, Out_CqueryCallTree::CallType::Derived);

  return result;
}

struct CqueryCallTreeInitialHandler
    : BaseMessageHandler<Ipc_CqueryCallTreeInitial> {
  void Run(Ipc_CqueryCallTreeInitial* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_CqueryCallTree out;
    out.id = request->id;

    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (sym.kind == SymbolKind::Func) {
        out.result =
            BuildInitialCallTree(db, working_files, QueryFuncId(sym.Idx()));
        break;
      }
    }

    QueueManager::WriteStdout(IpcId::CqueryCallTreeInitial, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallTreeInitialHandler);

struct CqueryCallTreeExpandHandler
    : BaseMessageHandler<Ipc_CqueryCallTreeExpand> {
  void Run(Ipc_CqueryCallTreeExpand* request) override {
    Out_CqueryCallTree out;
    out.id = request->id;

    // FIXME
    Maybe<QueryFuncId> func_id =
        db->GetQueryFuncIdFromUsr(std::stoull(request->params.usr));
    if (func_id)
      out.result = BuildExpandCallTree(db, working_files, *func_id);

    QueueManager::WriteStdout(IpcId::CqueryCallTreeExpand, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallTreeExpandHandler);
}  // namespace
