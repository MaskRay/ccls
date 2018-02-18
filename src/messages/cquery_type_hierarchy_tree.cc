#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryTypeHierarchyTree
    : public RequestMessage<Ipc_CqueryTypeHierarchyTree> {
  const static IpcId kIpcId = IpcId::CqueryTypeHierarchyTree;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryTypeHierarchyTree, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryTypeHierarchyTree);

struct Out_CqueryTypeHierarchyTree
    : public lsOutMessage<Out_CqueryTypeHierarchyTree> {
  struct TypeEntry {
    std::string_view name;
    optional<lsLocation> location;
    std::vector<TypeEntry> children;
  };
  lsRequestId id;
  optional<TypeEntry> result;
};
MAKE_REFLECT_STRUCT(Out_CqueryTypeHierarchyTree::TypeEntry,
                    name,
                    location,
                    children);
MAKE_REFLECT_STRUCT(Out_CqueryTypeHierarchyTree, jsonrpc, id, result);

std::vector<Out_CqueryTypeHierarchyTree::TypeEntry>
BuildParentInheritanceHierarchyForType(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       QueryType& root_type) {
  std::vector<Out_CqueryTypeHierarchyTree::TypeEntry> parent_entries;
  const QueryType::Def* def = root_type.AnyDef();
  parent_entries.reserve(def->parents.size());

  EachWithGen(db->types, def->parents, [&](QueryType& parent_type) {
    Out_CqueryTypeHierarchyTree::TypeEntry parent_entry;
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

optional<Out_CqueryTypeHierarchyTree::TypeEntry>
BuildInheritanceHierarchyForType(QueryDatabase* db,
                                 WorkingFiles* working_files,
                                 QueryType& root_type) {
  Out_CqueryTypeHierarchyTree::TypeEntry entry;
  const QueryType::Def* def = root_type.AnyDef();

  // Name and location.
  entry.name = def->detailed_name;
  if (def->spell)
    entry.location = GetLsLocation(db, working_files, *def->spell);

  entry.children.reserve(root_type.derived.size());

  // Base types.
  Out_CqueryTypeHierarchyTree::TypeEntry base;
  base.name = "[[Base]]";
  base.location = entry.location;
  base.children =
    BuildParentInheritanceHierarchyForType(db, working_files, root_type);
  if (!base.children.empty())
    entry.children.push_back(base);

  // Add derived.
  EachWithGen(db->types, root_type.derived, [&](QueryType& type) {
    auto derived_entry =
        BuildInheritanceHierarchyForType(db, working_files, type);
    if (derived_entry)
      entry.children.push_back(*derived_entry);
  });

  return entry;
}

std::vector<Out_CqueryTypeHierarchyTree::TypeEntry>
BuildParentInheritanceHierarchyForFunc(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       QueryFuncId root) {
  std::vector<Out_CqueryTypeHierarchyTree::TypeEntry> entries;

  QueryFunc& root_func = db->funcs[root.id];
  const QueryFunc::Def* def = root_func.AnyDef();
  if (!def || def->base.empty())
    return {};

  for (auto parent_id : def->base) {
    QueryFunc& parent_func = db->funcs[parent_id.id];
    const QueryFunc::Def* def1 = parent_func.AnyDef();
    if (!def1)
      continue;

    Out_CqueryTypeHierarchyTree::TypeEntry parent_entry;
    parent_entry.name = def1->detailed_name;
    if (def1->spell)
      parent_entry.location = GetLsLocation(db, working_files, *def1->spell);
    parent_entry.children =
        BuildParentInheritanceHierarchyForFunc(db, working_files, parent_id);

    entries.push_back(parent_entry);
  }

  return entries;
}

optional<Out_CqueryTypeHierarchyTree::TypeEntry>
BuildInheritanceHierarchyForFunc(QueryDatabase* db,
                                 WorkingFiles* working_files,
                                 QueryFuncId root_id) {
  QueryFunc& root_func = db->funcs[root_id.id];
  const QueryFunc::Def* def = root_func.AnyDef();
  if (!def)
    return nullopt;

  Out_CqueryTypeHierarchyTree::TypeEntry entry;

  // Name and location.
  entry.name = def->detailed_name;
  if (def->spell)
    entry.location =
        GetLsLocation(db, working_files, *def->spell);

  entry.children.reserve(root_func.derived.size());

  // Base types.
  Out_CqueryTypeHierarchyTree::TypeEntry base;
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

struct CqueryTypeHierarchyTreeHandler
    : BaseMessageHandler<Ipc_CqueryTypeHierarchyTree> {
  void Run(Ipc_CqueryTypeHierarchyTree* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_CqueryTypeHierarchyTree out;
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

    QueueManager::WriteStdout(IpcId::CqueryTypeHierarchyTree, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryTypeHierarchyTreeHandler);
}  // namespace
