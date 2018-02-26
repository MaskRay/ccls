#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryInheritanceHierarchyInitial
    : public RequestMessage<Ipc_CqueryInheritanceHierarchyInitial> {
  const static IpcId kIpcId = IpcId::CqueryInheritanceHierarchyInitial;
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    // true: derived classes/functions; false: base classes/functions
    bool derived = false;
    bool detailedName = false;
    int levels = 1;
  };
  Params params;
};

MAKE_REFLECT_STRUCT(Ipc_CqueryInheritanceHierarchyInitial::Params,
                    textDocument,
                    position,
                    derived,
                    detailedName,
                    levels);
MAKE_REFLECT_STRUCT(Ipc_CqueryInheritanceHierarchyInitial, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryInheritanceHierarchyInitial);

struct Ipc_CqueryInheritanceHierarchyExpand
    : public RequestMessage<Ipc_CqueryInheritanceHierarchyExpand> {
  const static IpcId kIpcId = IpcId::CqueryInheritanceHierarchyExpand;
  struct Params {
    Maybe<Id<void>> id;
    SymbolKind kind = SymbolKind::Invalid;
    bool derived = false;
    bool detailedName = false;
    int levels = 1;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryInheritanceHierarchyExpand::Params,
                    id,
                    kind,
                    derived,
                    detailedName,
                    levels);
MAKE_REFLECT_STRUCT(Ipc_CqueryInheritanceHierarchyExpand, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryInheritanceHierarchyExpand);

struct Out_CqueryInheritanceHierarchy
    : public lsOutMessage<Out_CqueryInheritanceHierarchy> {
  struct Entry {
    Id<void> id;
    SymbolKind kind;
    std::string_view name;
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
MAKE_REFLECT_STRUCT(Out_CqueryInheritanceHierarchy::Entry,
                    id,
                    kind,
                    name,
                    location,
                    numChildren,
                    children);
MAKE_REFLECT_STRUCT(Out_CqueryInheritanceHierarchy, jsonrpc, id, result);

bool Expand(MessageHandler* m,
            Out_CqueryInheritanceHierarchy::Entry* entry,
            bool derived,
            bool detailed_name,
            int levels);

template <typename Q>
bool ExpandHelper(MessageHandler* m,
                  Out_CqueryInheritanceHierarchy::Entry* entry,
                  bool derived,
                  bool detailed_name,
                  int levels,
                  Q& entity) {
  const auto* def = entity.AnyDef();
  if (!def) {
    entry->numChildren = 0;
    return false;
  }
  if (detailed_name)
    entry->name = def->DetailedName(false);
  else
    entry->name = def->ShortName();
  if (def->spell) {
    if (optional<lsLocation> loc =
            GetLsLocation(m->db, m->working_files, *def->spell))
      entry->location = *loc;
  }
  if (derived) {
    entry->numChildren = int(entity.derived.size());
    if (levels > 0) {
      for (auto id : entity.derived) {
        Out_CqueryInheritanceHierarchy::Entry entry1;
        entry1.id = id;
        entry1.kind = entry->kind;
        if (Expand(m, &entry1, derived, detailed_name, levels - 1))
          entry->children.push_back(std::move(entry1));
      }
      entry->numChildren = int(entry->children.size());
    }
  } else {
    entry->numChildren = int(def->bases.size());
    if (levels > 0) {
      for (auto id : def->bases) {
        Out_CqueryInheritanceHierarchy::Entry entry1;
        entry1.id = id;
        entry1.kind = entry->kind;
        if (Expand(m, &entry1, derived, detailed_name, levels - 1))
          entry->children.push_back(std::move(entry1));
      }
      entry->numChildren = int(entry->children.size());
    }
  }
  return true;
}

bool Expand(MessageHandler* m,
            Out_CqueryInheritanceHierarchy::Entry* entry,
            bool derived,
            bool detailed_name,
            int levels) {
  if (entry->kind == SymbolKind::Func)
    return ExpandHelper(m, entry, derived, detailed_name, levels,
                        m->db->funcs[entry->id.id]);
  else
    return ExpandHelper(m, entry, derived, detailed_name, levels,
                        m->db->types[entry->id.id]);
}

struct CqueryInheritanceHierarchyInitialHandler
    : BaseMessageHandler<Ipc_CqueryInheritanceHierarchyInitial> {
  optional<Out_CqueryInheritanceHierarchy::Entry>
  BuildInitial(SymbolRef sym, bool derived, bool detailed_name, int levels) {
    Out_CqueryInheritanceHierarchy::Entry entry;
    entry.id = sym.id;
    entry.kind = sym.kind;
    Expand(this, &entry, derived, detailed_name, levels);
    return entry;
  }

  void Run(Ipc_CqueryInheritanceHierarchyInitial* request) override {
    const auto& params = request->params;
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);
    Out_CqueryInheritanceHierarchy out;
    out.id = request->id;

    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (sym.kind == SymbolKind::Func || sym.kind == SymbolKind::Type) {
        out.result = BuildInitial(sym, params.derived, params.detailedName,
                                  params.levels);
        break;
      }
    }

    QueueManager::WriteStdout(IpcId::CqueryInheritanceHierarchyInitial, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryInheritanceHierarchyInitialHandler);

struct CqueryInheritanceHierarchyExpandHandler
  : BaseMessageHandler<Ipc_CqueryInheritanceHierarchyExpand> {
  void Run(Ipc_CqueryInheritanceHierarchyExpand* request) override {
    const auto& params = request->params;
    Out_CqueryInheritanceHierarchy out;
    out.id = request->id;
    if (params.id) {
      Out_CqueryInheritanceHierarchy::Entry entry;
      entry.id = *params.id;
      entry.kind = params.kind;
      if (((entry.kind == SymbolKind::Func && entry.id.id < db->funcs.size()) ||
           (entry.kind == SymbolKind::Type &&
            entry.id.id < db->types.size())) &&
          Expand(this, &entry, params.derived, params.detailedName,
                 params.levels))
        out.result = std::move(entry);
    }

    QueueManager::WriteStdout(IpcId::CqueryInheritanceHierarchyExpand, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryInheritanceHierarchyExpandHandler);
}  // namespace
