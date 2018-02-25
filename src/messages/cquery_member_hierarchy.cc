#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryMemberHierarchyInitial
    : public RequestMessage<Ipc_CqueryMemberHierarchyInitial> {
  const static IpcId kIpcId = IpcId::CqueryMemberHierarchyInitial;
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    int levels = 1;
  };
  Params params;
};

MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyInitial::Params,
                    textDocument,
                    position,
                    levels);
MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyInitial, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryMemberHierarchyInitial);

struct Ipc_CqueryMemberHierarchyExpand
    : public RequestMessage<Ipc_CqueryMemberHierarchyExpand> {
  const static IpcId kIpcId = IpcId::CqueryMemberHierarchyExpand;
  struct Params {
    Maybe<QueryTypeId> id;
    int levels = 1;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyExpand::Params, id, levels);
MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyExpand, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryMemberHierarchyExpand);

struct Out_CqueryMemberHierarchy
    : public lsOutMessage<Out_CqueryMemberHierarchy> {
  struct Entry {
    QueryTypeId id;
    std::string_view name;
    lsLocation location;
    int numChildren;
    // Empty if the |levels| limit is reached.
    std::vector<Entry> children;
  };
  lsRequestId id;
  optional<Entry> result;
};
MAKE_REFLECT_STRUCT(Out_CqueryMemberHierarchy::Entry,
                    id,
                    name,
                    location,
                    numChildren,
                    children);
MAKE_REFLECT_STRUCT(Out_CqueryMemberHierarchy, jsonrpc, id, result);

void Expand(MessageHandler* m, Out_CqueryMemberHierarchy::Entry* entry, int levels) {
  const QueryType::Def* def = m->db->types[entry->id.id].AnyDef();
  if (!def) {
    entry->numChildren = 0;
    return;
  }
  if (def->spell) {
    if (optional<lsLocation> loc =
        GetLsLocation(m->db, m->working_files, *def->spell))
      entry->location = *loc;
  }
  entry->numChildren = int(def->vars.size());
  if (levels > 0) {
    EachDefinedEntity(m->db->vars, def->vars, [&](QueryVar& var) {
        const QueryVar::Def* def1 = var.AnyDef();
        Out_CqueryMemberHierarchy::Entry entry1;
        entry1.name = def1->ShortName();
        entry1.id = def1->type ? *def1->type : QueryTypeId();
        Expand(m, &entry1, levels - 1);
        entry->children.push_back(std::move(entry1));
      });
  }
}

struct CqueryMemberHierarchyInitialHandler
    : BaseMessageHandler<Ipc_CqueryMemberHierarchyInitial> {
  optional<Out_CqueryMemberHierarchy::Entry> BuildInitial(QueryTypeId root_id,
                                                          int levels) {
    const auto* def = db->types[root_id.id].AnyDef();
    if (!def)
      return {};

    Out_CqueryMemberHierarchy::Entry entry;
    entry.id = root_id;
    entry.name = def->ShortName();
    Expand(this, &entry, levels);
    return entry;
  }

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
        out.result = BuildInitial(QueryTypeId(sym.id), request->params.levels);
        break;
      }
      if (sym.kind == SymbolKind::Var) {
        const QueryVar::Def* def = db->GetVar(sym).AnyDef();
        if (def && def->type)
          out.result = BuildInitial(QueryTypeId(*def->type), request->params.levels);
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
    if (request->params.id) {
      Out_CqueryMemberHierarchy::Entry entry;
      entry.id = *request->params.id;
      // entry.name is empty and it is known by the client.
      if (entry.id.id < db->types.size())
        Expand(this, &entry, request->params.levels);
      out.result = std::move(entry);
    }

    QueueManager::WriteStdout(IpcId::CqueryMemberHierarchyExpand, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryMemberHierarchyExpandHandler);
}  // namespace
