#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryInheritanceHierarchy
    : public RequestMessage<Ipc_CqueryInheritanceHierarchy> {
  const static IpcId kIpcId = IpcId::CqueryInheritanceHierarchy;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryInheritanceHierarchy, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryInheritanceHierarchy);

struct Out_CqueryInheritanceHierarchy
    : public lsOutMessage<Out_CqueryInheritanceHierarchy> {
  struct TypeEntry {
    std::string_view name;
    optional<lsLocation> location;
    std::vector<TypeEntry> children;
  };
  lsRequestId id;
  optional<TypeEntry> result;
};
MAKE_REFLECT_STRUCT(Out_CqueryInheritanceHierarchy::TypeEntry,
                    name,
                    location,
                    children);
MAKE_REFLECT_STRUCT(Out_CqueryInheritanceHierarchy, jsonrpc, id, result);

std::vector<Out_CqueryInheritanceHierarchy::TypeEntry>
BuildParentInheritanceHierarchyForType(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       QueryType& root_type) {
  std::vector<Out_CqueryInheritanceHierarchy::TypeEntry> parent_entries;
  const QueryType::Def* def = root_type.AnyDef();
  parent_entries.reserve(def->parents.size());

  EachDefinedEntity(db->types, def->parents, [&](QueryType& parent_type) {
    Out_CqueryInheritanceHierarchy::TypeEntry parent_entry;
    const QueryType::Def* def1 = parent_type.AnyDef();
    parent_entry.name = def1->detailed_name.c_str();
    if (def1->spell)
      parent_entry.location = GetLsLocation(db, working_files, *def1->spell);
    parent_entry.children =
        BuildParentInheritanceHierarchyForType(db, working_files, parent_type);

    parent_entries.push_back(parent_entry);
  });

  return parent_entries;
}

optional<Out_CqueryInheritanceHierarchy::TypeEntry>
BuildInheritanceHierarchyForType(QueryDatabase* db,
                                 WorkingFiles* working_files,
                                 QueryType& root_type) {
  Out_CqueryInheritanceHierarchy::TypeEntry entry;
  const QueryType::Def* def = root_type.AnyDef();

  // Name and location.
  entry.name = def->detailed_name;
  if (def->spell)
    entry.location = GetLsLocation(db, working_files, *def->spell);

  entry.children.reserve(root_type.derived.size());

  // Base types.
  Out_CqueryInheritanceHierarchy::TypeEntry base;
  base.name = "[[Base]]";
  base.location = entry.location;
  base.children =
      BuildParentInheritanceHierarchyForType(db, working_files, root_type);
  if (!base.children.empty())
    entry.children.push_back(base);

  // Add derived.
  EachDefinedEntity(db->types, root_type.derived, [&](QueryType& type) {
    auto derived_entry =
        BuildInheritanceHierarchyForType(db, working_files, type);
    if (derived_entry)
      entry.children.push_back(*derived_entry);
  });

  return entry;
}

std::vector<Out_CqueryInheritanceHierarchy::TypeEntry>
BuildParentInheritanceHierarchyForFunc(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       QueryFuncId root) {
  std::vector<Out_CqueryInheritanceHierarchy::TypeEntry> entries;

  QueryFunc& root_func = db->funcs[root.id];
  const QueryFunc::Def* def = root_func.AnyDef();
  if (!def || def->base.empty())
    return {};

  for (auto parent_id : def->base) {
    QueryFunc& parent_func = db->funcs[parent_id.id];
    const QueryFunc::Def* def1 = parent_func.AnyDef();
    if (!def1)
      continue;

    Out_CqueryInheritanceHierarchy::TypeEntry parent_entry;
    parent_entry.name = def1->detailed_name;
    if (def1->spell)
      parent_entry.location = GetLsLocation(db, working_files, *def1->spell);
    parent_entry.children =
        BuildParentInheritanceHierarchyForFunc(db, working_files, parent_id);

    entries.push_back(parent_entry);
  }

  return entries;
}

optional<Out_CqueryInheritanceHierarchy::TypeEntry>
BuildInheritanceHierarchyForFunc(QueryDatabase* db,
                                 WorkingFiles* working_files,
                                 QueryFuncId root_id) {
  QueryFunc& root_func = db->funcs[root_id.id];
  const QueryFunc::Def* def = root_func.AnyDef();
  if (!def)
    return nullopt;

  Out_CqueryInheritanceHierarchy::TypeEntry entry;

  // Name and location.
  entry.name = def->detailed_name;
  if (def->spell)
    entry.location = GetLsLocation(db, working_files, *def->spell);

  entry.children.reserve(root_func.derived.size());

  // Base types.
  Out_CqueryInheritanceHierarchy::TypeEntry base;
  base.name = "[[Base]]";
  base.location = entry.location;
  base.children =
      BuildParentInheritanceHierarchyForFunc(db, working_files, root_id);
  if (!base.children.empty())
    entry.children.push_back(base);

  // Add derived.
  for (auto derived : root_func.derived) {
    auto derived_entry =
        BuildInheritanceHierarchyForFunc(db, working_files, derived);
    if (derived_entry)
      entry.children.push_back(*derived_entry);
  }

  return entry;
}

struct CqueryInheritanceHierarchyHandler
    : BaseMessageHandler<Ipc_CqueryInheritanceHierarchy> {
  void Run(Ipc_CqueryInheritanceHierarchy* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_CqueryInheritanceHierarchy out;
    out.id = request->id;

    for (const SymbolRef& sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (sym.kind == SymbolKind::Type) {
        QueryType& type = db->GetType(sym);
        if (type.AnyDef())
          out.result =
              BuildInheritanceHierarchyForType(db, working_files, type);
        break;
      }
      if (sym.kind == SymbolKind::Func) {
        out.result = BuildInheritanceHierarchyForFunc(db, working_files,
                                                      QueryFuncId(sym.id));
        break;
      }
    }

    QueueManager::WriteStdout(IpcId::CqueryInheritanceHierarchy, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryInheritanceHierarchyHandler);
}  // namespace
