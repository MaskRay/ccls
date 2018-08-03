#include "message_handler.h"
#include "query_utils.h"
#include "pipeline.hh"
#include "log.hh"

using namespace ccls;

#include <unordered_set>

namespace {
MethodType kMethodType = "$ccls/inheritanceHierarchy";

struct In_CclsInheritanceHierarchy : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    // If id+kind are specified, expand a node; otherwise textDocument+position
    // should be specified for building the root and |levels| of nodes below.
    lsTextDocumentIdentifier textDocument;
    lsPosition position;

    Usr usr;
    std::string id;
    SymbolKind kind = SymbolKind::Invalid;

    // true: derived classes/functions; false: base classes/functions
    bool derived = false;
    bool qualified = true;
    int levels = 1;
    bool flat = false;
  };
  Params params;
};

MAKE_REFLECT_STRUCT(In_CclsInheritanceHierarchy::Params,
                    textDocument,
                    position,
                    id,
                    kind,
                    derived,
                    qualified,
                    levels,
                    flat);
MAKE_REFLECT_STRUCT(In_CclsInheritanceHierarchy, id, params);
REGISTER_IN_MESSAGE(In_CclsInheritanceHierarchy);

struct Out_CclsInheritanceHierarchy
    : public lsOutMessage<Out_CclsInheritanceHierarchy> {
  struct Entry {
    Usr usr;
    std::string id;
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
  std::optional<Entry> result;
};
MAKE_REFLECT_STRUCT(Out_CclsInheritanceHierarchy::Entry,
                    id,
                    kind,
                    name,
                    location,
                    numChildren,
                    children);
MAKE_REFLECT_STRUCT_MANDATORY_OPTIONAL(Out_CclsInheritanceHierarchy,
                                       jsonrpc,
                                       id,
                                       result);

bool Expand(MessageHandler* m,
            Out_CclsInheritanceHierarchy::Entry* entry,
            bool derived,
            bool qualified,
            int levels);

template <typename Q>
bool ExpandHelper(MessageHandler* m,
                  Out_CclsInheritanceHierarchy::Entry* entry,
                  bool derived,
                  bool qualified,
                  int levels,
                  Q& entity) {
  const auto* def = entity.AnyDef();
  if (def) {
    entry->name = def->Name(qualified);
    if (def->spell) {
      if (auto loc = GetLsLocation(m->db, m->working_files, *def->spell))
        entry->location = *loc;
    }
  } else if (!derived) {
    entry->numChildren = 0;
    return false;
  }
  std::unordered_set<Usr> seen;
  if (derived) {
    if (levels > 0) {
      for (auto usr : entity.derived) {
        if (!seen.insert(usr).second)
          continue;
        Out_CclsInheritanceHierarchy::Entry entry1;
        entry1.id = std::to_string(usr);
        entry1.usr = usr;
        entry1.kind = entry->kind;
        if (Expand(m, &entry1, derived, qualified, levels - 1))
          entry->children.push_back(std::move(entry1));
      }
      entry->numChildren = int(entry->children.size());
    } else
      entry->numChildren = int(entity.derived.size());
  } else {
    if (levels > 0) {
      for (auto usr : def->bases) {
        if (!seen.insert(usr).second)
          continue;
        Out_CclsInheritanceHierarchy::Entry entry1;
        entry1.id = std::to_string(usr);
        entry1.usr = usr;
        entry1.kind = entry->kind;
        if (Expand(m, &entry1, derived, qualified, levels - 1))
          entry->children.push_back(std::move(entry1));
      }
      entry->numChildren = int(entry->children.size());
    } else
      entry->numChildren = int(def->bases.size());
  }
  return true;
}

bool Expand(MessageHandler* m,
            Out_CclsInheritanceHierarchy::Entry* entry,
            bool derived,
            bool qualified,
            int levels) {
  if (entry->kind == SymbolKind::Func)
    return ExpandHelper(m, entry, derived, qualified, levels,
                        m->db->Func(entry->usr));
  else
    return ExpandHelper(m, entry, derived, qualified, levels,
                        m->db->Type(entry->usr));
}

struct Handler_CclsInheritanceHierarchy
    : BaseMessageHandler<In_CclsInheritanceHierarchy> {
  MethodType GetMethodType() const override { return kMethodType; }

  std::optional<Out_CclsInheritanceHierarchy::Entry>
  BuildInitial(SymbolRef sym, bool derived, bool qualified, int levels) {
    Out_CclsInheritanceHierarchy::Entry entry;
    entry.id = std::to_string(sym.usr);
    entry.usr = sym.usr;
    entry.kind = sym.kind;
    Expand(this, &entry, derived, qualified, levels);
    return entry;
  }

  void Run(In_CclsInheritanceHierarchy* request) override {
    auto& params = request->params;
    Out_CclsInheritanceHierarchy out;
    out.id = request->id;
    bool flat = params.flat;

    if (params.id.size()) {
      if (flat) return;
      try {
        params.usr = std::stoull(params.id);
      } catch (...) {
        return;
      }
      Out_CclsInheritanceHierarchy::Entry entry;
      entry.id = std::to_string(params.usr);
      entry.usr = params.usr;
      entry.kind = params.kind;
      if (((entry.kind == SymbolKind::Func && db->HasFunc(entry.usr)) ||
           (entry.kind == SymbolKind::Type && db->HasType(entry.usr))) &&
          Expand(this, &entry, params.derived, params.qualified, params.levels))
        out.result = std::move(entry);
    } else {
      QueryFile* file;
      if (!FindFileOrFail(db, project, request->id,
                          params.textDocument.uri.GetPath(), &file))
        return;
      WorkingFile* wfile =
          working_files->GetFileByFilename(file->def->path);

      for (SymbolRef sym : FindSymbolsAtLocation(wfile, file, params.position))
        if (sym.kind == SymbolKind::Func || sym.kind == SymbolKind::Type) {
          out.result = BuildInitial(sym, params.derived, params.qualified,
                                    params.levels);
          break;
        }
    }
    if (flat) {
      Out_LocationList out2;
      out2.id = request->id;
      if (out.result) {
        auto push = [&out2](Out_CclsInheritanceHierarchy::Entry & entry) {
          auto y = [&out2](Out_CclsInheritanceHierarchy::Entry & entry, auto& z)
                   -> void {
            if (entry.location.uri.GetPath().size())
              out2.result.push_back({entry.location});
            for (auto & child : entry.children) {
              z(child, z);
            }
          };
          y(entry, y);
        };
        push(*out.result);
      }
      pipeline::WriteStdout(kMethodType, out2);
    } else
      pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsInheritanceHierarchy);

}  // namespace
