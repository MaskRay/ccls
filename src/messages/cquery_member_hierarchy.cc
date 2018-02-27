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
    bool detailedName = false;
    int levels = 1;
  };
  Params params;
};

MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyInitial::Params,
                    textDocument,
                    position,
                    detailedName,
                    levels);
MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyInitial, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryMemberHierarchyInitial);

struct Ipc_CqueryMemberHierarchyExpand
    : public RequestMessage<Ipc_CqueryMemberHierarchyExpand> {
  const static IpcId kIpcId = IpcId::CqueryMemberHierarchyExpand;
  struct Params {
    Maybe<QueryTypeId> id;
    bool detailedName = false;
    int levels = 1;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyExpand::Params, id, detailedName, levels);
MAKE_REFLECT_STRUCT(Ipc_CqueryMemberHierarchyExpand, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryMemberHierarchyExpand);

struct Out_CqueryMemberHierarchy
    : public lsOutMessage<Out_CqueryMemberHierarchy> {
  struct Entry {
    QueryTypeId id;
    std::string_view name;
    std::string fieldName;
    lsLocation location;
    // For unexpanded nodes, this is an upper bound because some entities may be
    // undefined. If it is 0, there are no members.
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
                    fieldName,
                    location,
                    numChildren,
                    children);
MAKE_REFLECT_STRUCT(Out_CqueryMemberHierarchy, jsonrpc, id, result);

bool Expand(MessageHandler* m,
            Out_CqueryMemberHierarchy::Entry* entry,
            bool detailed_name,
            int levels) {
  const QueryType& type = m->db->types[entry->id.id];
  const QueryType::Def* def = type.AnyDef();
  // builtin types have no declaration and empty |detailed_name|.
  if (CXType_FirstBuiltin <= type.usr && type.usr <= CXType_LastBuiltin) {
    entry->name = ClangBuiltinTypeName(CXTypeKind(type.usr));
    entry->numChildren = 0;
    return true;
  }
  if (!def) {
    entry->numChildren = 0;
    return false;
  }
  if (detailed_name)
    entry->name = def->detailed_name;
  else
    entry->name = def->ShortName();
  std::unordered_set<Usr> seen;
  if (levels > 0) {
    std::vector<const QueryType*> stack;
    seen.insert(type.usr);
    stack.push_back(&type);
    while (stack.size()) {
      const auto* def = stack.back()->AnyDef();
      stack.pop_back();
      if (def) {
        EachDefinedEntity(m->db->types, def->bases, [&](QueryType& type1) {
          if (!seen.count(type1.usr)) {
            seen.insert(type1.usr);
            stack.push_back(&type1);
          }
        });
        if (def->alias_of) {
          const QueryType::Def* def1 = m->db->types[def->alias_of->id].AnyDef();
          if (!def1)
            continue;
          Out_CqueryMemberHierarchy::Entry entry1;
          entry1.id = *def->alias_of;
          if (def1->spell) {
            if (optional<lsLocation> loc =
                GetLsLocation(m->db, m->working_files, *def1->spell))
              entry1.location = *loc;
          }
          if (detailed_name)
            entry1.fieldName = def1->detailed_name;
          if (Expand(m, &entry1, detailed_name, levels - 1))
            entry->children.push_back(std::move(entry1));
        } else {
          EachDefinedEntity(m->db->vars, def->vars, [&](QueryVar& var) {
            const QueryVar::Def* def1 = var.AnyDef();
            if (!def1)
              return;
            Out_CqueryMemberHierarchy::Entry entry1;
            entry1.id = def1->type ? *def1->type : QueryTypeId();
            if (detailed_name)
              entry1.fieldName = def1->DetailedName(false);
            else
              entry1.fieldName = std::string(def1->ShortName());
            if (def1->spell) {
              if (optional<lsLocation> loc =
                  GetLsLocation(m->db, m->working_files, *def1->spell))
                entry1.location = *loc;
            }
            if (def1->type && Expand(m, &entry1, detailed_name, levels - 1))
              entry->children.push_back(std::move(entry1));
          });
        }
      }
    }
    entry->numChildren = int(entry->children.size());
  } else
    entry->numChildren = def->alias_of ? 1 : int(def->vars.size());
  return true;
}

struct CqueryMemberHierarchyInitialHandler
    : BaseMessageHandler<Ipc_CqueryMemberHierarchyInitial> {
  optional<Out_CqueryMemberHierarchy::Entry> BuildInitial(QueryTypeId root_id,
                                                          bool detailed_name,
                                                          int levels) {
    const auto* def = db->types[root_id.id].AnyDef();
    if (!def)
      return {};

    Out_CqueryMemberHierarchy::Entry entry;
    entry.id = root_id;
    if (def->spell) {
      if (optional<lsLocation> loc =
          GetLsLocation(db, working_files, *def->spell))
        entry.location = *loc;
    }
    Expand(this, &entry, detailed_name, levels);
    return entry;
  }

  void Run(Ipc_CqueryMemberHierarchyInitial* request) override {
    QueryFile* file;
    const auto& params = request->params;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);
    Out_CqueryMemberHierarchy out;
    out.id = request->id;

    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, params.position)) {
      if (sym.kind == SymbolKind::Type) {
        out.result = BuildInitial(QueryTypeId(sym.id), params.detailedName,
                                  params.levels);
        break;
      }
      if (sym.kind == SymbolKind::Var) {
        const QueryVar::Def* def = db->GetVar(sym).AnyDef();
        if (def && def->type)
          out.result = BuildInitial(QueryTypeId(*def->type),
                                    params.detailedName, params.levels);
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
    const auto& params = request->params;
    Out_CqueryMemberHierarchy out;
    out.id = request->id;
    if (params.id) {
      Out_CqueryMemberHierarchy::Entry entry;
      entry.id = *request->params.id;
      // entry.name is empty as it is known by the client.
      if (entry.id.id < db->types.size() &&
          Expand(this, &entry, params.detailedName, params.levels))
        out.result = std::move(entry);
    }

    QueueManager::WriteStdout(IpcId::CqueryMemberHierarchyExpand, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryMemberHierarchyExpandHandler);
}  // namespace
