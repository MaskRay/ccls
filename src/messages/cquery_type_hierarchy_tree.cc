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
    std::string name;
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
  parent_entries.reserve(root_type.def->parents.size());

  EachWithGen(db->types, root_type.def->parents, [&](QueryType& parent_type) {
    Out_CqueryTypeHierarchyTree::TypeEntry parent_entry;
    parent_entry.name = parent_type.def->detailed_name;
    if (parent_type.def->definition_spelling)
      parent_entry.location = GetLsLocation(
          db, working_files, *parent_type.def->definition_spelling);
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

  // Name and location.
  entry.name = root_type.def->detailed_name;
  if (root_type.def->definition_spelling)
    entry.location =
        GetLsLocation(db, working_files, *root_type.def->definition_spelling);

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
  if (!root_func.def || root_func.def->base.empty())
    return {};

  // FIXME WithGen
  for (auto parent_id : root_func.def->base) {
    QueryFunc& parent_func = db->funcs[parent_id.id];
    if (!parent_func.def)
      continue;

    Out_CqueryTypeHierarchyTree::TypeEntry parent_entry;
    parent_entry.name = parent_func.def->detailed_name;
    if (parent_func.def->definition_spelling)
      parent_entry.location = GetLsLocation(
          db, working_files, *parent_func.def->definition_spelling);
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
  if (!root_func.def)
    return nullopt;

  Out_CqueryTypeHierarchyTree::TypeEntry entry;

  // Name and location.
  entry.name = root_func.def->detailed_name;
  if (root_func.def->definition_spelling)
    entry.location =
        GetLsLocation(db, working_files, *root_func.def->definition_spelling);

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

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (ref.idx.kind == SymbolKind::Type) {
        QueryType& type = db->types[ref.idx.idx];
        if (type.def)
          out.result =
              BuildInheritanceHierarchyForType(db, working_files, type);
        break;
      }
      if (ref.idx.kind == SymbolKind::Func) {
        out.result = BuildInheritanceHierarchyForFunc(db, working_files,
                                                      QueryFuncId(ref.idx.idx));
        break;
      }
    }

    QueueManager::WriteStdout(IpcId::CqueryTypeHierarchyTree, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryTypeHierarchyTreeHandler);
}  // namespace
