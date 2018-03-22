#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
MethodType kMethodType = "$cquery/inheritanceHierarchy";

struct In_CqueryInheritanceHierarchy : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    // If id+kind are specified, expand a node; otherwise textDocument+position
    // should be specified for building the root and |levels| of nodes below.
    lsTextDocumentIdentifier textDocument;
    lsPosition position;

    Maybe<Id<void>> id;
    SymbolKind kind = SymbolKind::Invalid;

    // true: derived classes/functions; false: base classes/functions
    bool derived = false;
    bool detailedName = false;
    int levels = 1;
  };
  Params params;
};

MAKE_REFLECT_STRUCT(In_CqueryInheritanceHierarchy::Params,
                    textDocument,
                    position,
                    id,
                    kind,
                    derived,
                    detailedName,
                    levels);
MAKE_REFLECT_STRUCT(In_CqueryInheritanceHierarchy, id, params);
REGISTER_IN_MESSAGE(In_CqueryInheritanceHierarchy);

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
    if (levels > 0) {
      for (auto id : entity.derived) {
        Out_CqueryInheritanceHierarchy::Entry entry1;
        entry1.id = id;
        entry1.kind = entry->kind;
        if (Expand(m, &entry1, derived, detailed_name, levels - 1))
          entry->children.push_back(std::move(entry1));
      }
      entry->numChildren = int(entry->children.size());
    } else
      entry->numChildren = int(entity.derived.size());
  } else {
    if (levels > 0) {
      for (auto id : def->bases) {
        Out_CqueryInheritanceHierarchy::Entry entry1;
        entry1.id = id;
        entry1.kind = entry->kind;
        if (Expand(m, &entry1, derived, detailed_name, levels - 1))
          entry->children.push_back(std::move(entry1));
      }
      entry->numChildren = int(entry->children.size());
    } else
      entry->numChildren = int(def->bases.size());
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

struct Handler_CqueryInheritanceHierarchy
    : BaseMessageHandler<In_CqueryInheritanceHierarchy> {
  MethodType GetMethodType() const override { return kMethodType; }

  optional<Out_CqueryInheritanceHierarchy::Entry>
  BuildInitial(SymbolRef sym, bool derived, bool detailed_name, int levels) {
    Out_CqueryInheritanceHierarchy::Entry entry;
    entry.id = sym.id;
    entry.kind = sym.kind;
    Expand(this, &entry, derived, detailed_name, levels);
    return entry;
  }

  void Run(In_CqueryInheritanceHierarchy* request) override {
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
    } else {
      QueryFile* file;
      if (!FindFileOrFail(db, project, request->id,
                          params.textDocument.uri.GetPath(), &file))
        return;
      WorkingFile* working_file =
          working_files->GetFileByFilename(file->def->path);

      for (SymbolRef sym : FindSymbolsAtLocation(working_file, file,
                                                 request->params.position)) {
        if (sym.kind == SymbolKind::Func || sym.kind == SymbolKind::Type) {
          out.result = BuildInitial(sym, params.derived, params.detailedName,
                                    params.levels);
          break;
        }
      }
    }

    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryInheritanceHierarchy);

}  // namespace
