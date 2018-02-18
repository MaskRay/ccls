#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryMemberHierarchyInitial
    : public RequestMessage<Ipc_CqueryMemberHierarchyInitial> {
  const static IpcId kIpcId = IpcId::CqueryMemberHierarchyInitial;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyInitial, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryMemberHierarchyInitial);

struct Ipc_CqueryMemberHierarchyExpand
    : public RequestMessage<Ipc_CqueryMemberHierarchyExpand> {
  const static IpcId kIpcId = IpcId::CqueryMemberHierarchyExpand;
  struct Params {
    Maybe<QueryTypeId> type_id;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyExpand::Params, type_id);
MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyExpand, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryMemberHierarchyExpand);

struct Out_CqueryMemberHierarchy
    : public lsOutMessage<Out_CqueryMemberHierarchy> {
  struct Entry {
    std::string_view name;
    QueryTypeId type_id;
    lsLocation location;
  };
  lsRequestId id;
  std::vector<Entry> result;
};
MAKE_REFLECT_STRUCT(Out_CqueryMemberHierarchy::Entry, name, type_id, location);
MAKE_REFLECT_STRUCT(Out_CqueryMemberHierarchy, jsonrpc, id, result);

std::vector<Out_CqueryMemberHierarchy::Entry>
BuildInitial(QueryDatabase* db, WorkingFiles* working_files, QueryTypeId root) {
  const auto* root_type = db->types[root.id].AnyDef();
  if (!root_type || !root_type->spell)
    return {};
  optional<lsLocation> def_loc =
      GetLsLocation(db, working_files, *root_type->spell);
  if (!def_loc)
    return {};

  Out_CqueryMemberHierarchy::Entry entry;
  entry.type_id = root;
  entry.name = root_type->ShortName();
  entry.location = *def_loc;
  return {entry};
}

std::vector<Out_CqueryMemberHierarchy::Entry>
ExpandNode(QueryDatabase* db, WorkingFiles* working_files, QueryTypeId root) {
  QueryType& root_type = db->types[root.id];
  const QueryType::Def* def = root_type.AnyDef();
  if (!def)
    return {};

  std::vector<Out_CqueryMemberHierarchy::Entry> ret;
  EachWithGen(db->vars, def->vars, [&](QueryVar& var) {
    const QueryVar::Def* def1 = var.AnyDef();
    Out_CqueryMemberHierarchy::Entry entry;
    entry.name = def1->ShortName();
    entry.type_id = def1->type ? *def1->type : QueryTypeId();
    if (def->spell) {
      optional<lsLocation> loc =
          GetLsLocation(db, working_files, *def1->spell);
      // TODO invalid location
      if (loc)
        entry.location = *loc;
    }
    ret.push_back(std::move(entry));
  });
  return ret;
}

struct CqueryMemberHierarchyInitialHandler
    : BaseMessageHandler<Ipc_CqueryMemberHierarchyInitial> {
  void Run(Ipc_CqueryMemberHierarchyInitial* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);
    Out_CqueryMemberHierarchy out;
    out.id = request->id;

    for (const SymbolRef& sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (sym.kind == SymbolKind::Type) {
        out.result = BuildInitial(db, working_files, QueryTypeId(sym.id));
        break;
      }
      if (sym.kind == SymbolKind::Var) {
        const QueryVar::Def* def = db->GetVar(sym).AnyDef();
        if (def && def->type)
          out.result = BuildInitial(db, working_files, *def->type);
        break;
      }
    }

    QueueManager::WriteStdout(IpcId::CqueryMemberHierarchyInitial, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryMemberHierarchyInitialHandler);

struct CqueryMemberHierarchyExpandHandler
    : BaseMessageHandler<Ipc_CqueryMemberHierarchyExpand> {
  void Run(Ipc_CqueryMemberHierarchyExpand* request) override {
    Out_CqueryMemberHierarchy out;
    out.id = request->id;
    // |ExpandNode| uses -1 to indicate invalid |type_id|.
    if (request->params.type_id)
      out.result = ExpandNode(db, working_files, *request->params.type_id);

    QueueManager::WriteStdout(IpcId::CqueryMemberHierarchyExpand, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryMemberHierarchyExpandHandler);
}  // namespace
