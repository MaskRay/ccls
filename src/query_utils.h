#pragma once

#include "query.h"
#include "working_files.h"

#include <optional.h>

Maybe<Use> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                         SymbolIdx sym);
Maybe<Use> GetDefinitionExtentOfSymbol(QueryDatabase* db, SymbolIdx sym);
Maybe<QueryFileId> GetDeclarationFileForSymbol(QueryDatabase* db,
                                               SymbolIdx sym);

std::vector<Use> ToUses(QueryDatabase* db,
                        const std::vector<QueryFuncId>& ids);
std::vector<Use> ToUses(QueryDatabase* db,
                        const std::vector<QueryTypeId>& ids);
std::vector<Use> ToUses(QueryDatabase* db,
                        const std::vector<QueryVarId>& ids);

std::vector<Use> GetDeclarationsOfSymbolForGotoDefinition(
    QueryDatabase* db,
    SymbolIdx sym);

bool HasCallersOnSelfOrBaseOrDerived(QueryDatabase* db, QueryFunc& root);
std::vector<Use> GetCallersForAllBaseFunctions(QueryDatabase* db,
                                               QueryFunc& root);
std::vector<Use> GetCallersForAllDerivedFunctions(QueryDatabase* db,
                                                  QueryFunc& root);
optional<lsPosition> GetLsPosition(WorkingFile* working_file,
                                   const Position& position);
optional<lsRange> GetLsRange(WorkingFile* working_file, const Range& location);
lsDocumentUri GetLsDocumentUri(QueryDatabase* db,
                               QueryFileId file_id,
                               std::string* path);
lsDocumentUri GetLsDocumentUri(QueryDatabase* db, QueryFileId file_id);

optional<lsLocation> GetLsLocation(QueryDatabase* db,
                                   WorkingFiles* working_files,
                                   Use use);
optional<lsLocationEx> GetLsLocationEx(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       Use use,
                                       bool extension);
std::vector<lsLocation> GetLsLocations(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       const std::vector<Use>& refs,
                                       int limit);
// Returns a symbol. The symbol will have *NOT* have a location assigned.
optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolIdx sym,
                                            bool use_short_name);

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile* working_file,
                                             QueryFile* file,
                                             lsPosition position);

void EmitDiagnostics(WorkingFiles* working_files,
                     std::string path,
                     std::vector<lsDiagnostic> diagnostics);

template <typename Fn>
void WithEntity(QueryDatabase* db, SymbolIdx sym, Fn&& fn) {
  switch (sym.kind) {
    case SymbolKind::Invalid:
    case SymbolKind::File:
      break;
    case SymbolKind::Func:
      fn(db->GetFunc(sym));
      break;
    case SymbolKind::Type:
      fn(db->GetType(sym));
      break;
    case SymbolKind::Var:
      fn(db->GetVar(sym));
      break;
  }
}

template <typename Fn>
void EachEntityDef(QueryDatabase* db, SymbolIdx sym, Fn&& fn) {
  WithEntity(db, sym, [&](const auto& entity) {
    for (auto& def : entity.def)
      if (!fn(def))
        break;
  });
}

template <typename Fn>
void EachUse(QueryDatabase* db, SymbolIdx sym, bool include_decl, Fn&& fn) {
  WithEntity(db, sym, [&](const auto& entity) {
    for (Use use : entity.uses)
      fn(use);
    if (include_decl) {
      for (auto& def : entity.def)
        if (def.spell)
          fn(*def.spell);
      for (Use use : entity.declarations)
        fn(use);
    }
  });
}

template <typename Q, typename Fn>
void EachDefinedEntity(std::vector<Q>& collection, const std::vector<Id<Q>>& ids, Fn&& fn) {
  for (Id<Q> x : ids) {
    Q& obj = collection[x.id];
    if (!obj.def.empty())
      fn(obj);
  }
}
