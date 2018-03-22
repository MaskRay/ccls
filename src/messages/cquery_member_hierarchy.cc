#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
MethodType kMethodType = "$cquery/memberHierarchy";

struct In_CqueryMemberHierarchy : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  struct Params {
    // If id is specified, expand a node; otherwise textDocument+position should
    // be specified for building the root and |levels| of nodes below.
    lsTextDocumentIdentifier textDocument;
    lsPosition position;

    Maybe<QueryTypeId> id;

    bool detailedName = false;
    int levels = 1;
  };
  Params params;
};

MAKE_REFLECT_STRUCT(In_CqueryMemberHierarchy::Params,
                    textDocument,
                    position,
                    id,
                    detailedName,
                    levels);
MAKE_REFLECT_STRUCT(In_CqueryMemberHierarchy, id, params);
REGISTER_IN_MESSAGE(In_CqueryMemberHierarchy);

struct Out_CqueryMemberHierarchy
    : public lsOutMessage<Out_CqueryMemberHierarchy> {
  struct Entry {
    QueryTypeId id;
    std::string_view name;
    std::string fieldName;
    lsLocation location;
    // For unexpanded nodes, this is an upper bound because some entities may be
    // undefined. If it is 0, there are no members.
    int numChildren = 0;
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
            int levels);

// Add a field to |entry| which is a Func/Type.
void DoField(MessageHandler* m,
             Out_CqueryMemberHierarchy::Entry* entry,
             const QueryVar& var,
             bool detailed_name,
             int levels) {
  const QueryVar::Def* def1 = var.AnyDef();
  if (!def1)
    return;
  Out_CqueryMemberHierarchy::Entry entry1;
  if (detailed_name)
    entry1.fieldName = def1->DetailedName(false);
  else
    entry1.fieldName = std::string(def1->ShortName());
  if (def1->spell) {
    if (optional<lsLocation> loc =
            GetLsLocation(m->db, m->working_files, *def1->spell))
      entry1.location = *loc;
  }
  if (def1->type) {
    entry1.id = *def1->type;
    if (Expand(m, &entry1, detailed_name, levels))
      entry->children.push_back(std::move(entry1));
  } else {
    entry1.id = QueryTypeId();
    entry->children.push_back(std::move(entry1));
  }
}

// Expand a type node by adding members recursively to it.
bool Expand(MessageHandler* m,
            Out_CqueryMemberHierarchy::Entry* entry,
            bool detailed_name,
            int levels) {
  const QueryType& type = m->db->types[entry->id.id];
  const QueryType::Def* def = type.AnyDef();
  // builtin types have no declaration and empty |detailed_name|.
  if (CXType_FirstBuiltin <= type.usr && type.usr <= CXType_LastBuiltin) {
    entry->name = ClangBuiltinTypeName(CXTypeKind(type.usr));
    return true;
  }
  if (!def)
    return false;
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
          Out_CqueryMemberHierarchy::Entry entry1;
          entry1.id = *def->alias_of;
          if (def1 && def1->spell) {
            // The declaration of target type.
            if (optional<lsLocation> loc =
                    GetLsLocation(m->db, m->working_files, *def1->spell))
              entry1.location = *loc;
          } else if (def->spell) {
            // Builtin types have no declaration but the typedef declaration
            // itself is useful.
            if (optional<lsLocation> loc =
                    GetLsLocation(m->db, m->working_files, *def->spell))
              entry1.location = *loc;
          }
          if (def1 && detailed_name)
            entry1.fieldName = def1->detailed_name;
          if (Expand(m, &entry1, detailed_name, levels - 1)) {
            // For builtin types |name| is set.
            if (detailed_name && entry1.fieldName.empty())
              entry1.fieldName = std::string(entry1.name);
            entry->children.push_back(std::move(entry1));
          }
        } else {
          EachDefinedEntity(m->db->vars, def->vars, [&](QueryVar& var) {
            DoField(m, entry, var, detailed_name, levels - 1);
          });
        }
      }
    }
    entry->numChildren = int(entry->children.size());
  } else
    entry->numChildren = def->alias_of ? 1 : int(def->vars.size());
  return true;
}

struct Handler_CqueryMemberHierarchy
    : BaseMessageHandler<In_CqueryMemberHierarchy> {
  MethodType GetMethodType() const override { return kMethodType; }

  optional<Out_CqueryMemberHierarchy::Entry> BuildInitial(QueryFuncId root_id,
                                                          bool detailed_name,
                                                          int levels) {
    const auto* def = db->funcs[root_id.id].AnyDef();
    if (!def)
      return {};

    Out_CqueryMemberHierarchy::Entry entry;
    // Not type, |id| is invalid.
    if (detailed_name)
      entry.name = def->DetailedName(false);
    else
      entry.name = std::string(def->ShortName());
    if (def->spell) {
      if (optional<lsLocation> loc =
              GetLsLocation(db, working_files, *def->spell))
        entry.location = *loc;
    }
    EachDefinedEntity(db->vars, def->vars, [&](QueryVar& var) {
      DoField(this, &entry, var, detailed_name, levels - 1);
    });
    return entry;
  }

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

  void Run(In_CqueryMemberHierarchy* request) override {
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
    } else {
      QueryFile* file;
      if (!FindFileOrFail(db, project, request->id,
                          params.textDocument.uri.GetPath(), &file))
        return;
      WorkingFile* working_file =
          working_files->GetFileByFilename(file->def->path);
      for (SymbolRef sym :
           FindSymbolsAtLocation(working_file, file, params.position)) {
        switch (sym.kind) {
          case SymbolKind::Func:
            out.result = BuildInitial(QueryFuncId(sym.id), params.detailedName,
                                      params.levels);
            break;
          case SymbolKind::Type:
            out.result = BuildInitial(QueryTypeId(sym.id), params.detailedName,
                                      params.levels);
            break;
          case SymbolKind::Var: {
            const QueryVar::Def* def = db->GetVar(sym).AnyDef();
            if (def && def->type)
              out.result = BuildInitial(QueryTypeId(*def->type),
                                        params.detailedName, params.levels);
            break;
          }
          default:
            continue;
        }
        break;
      }
    }

    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryMemberHierarchy);

}  // namespace
