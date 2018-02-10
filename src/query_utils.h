#pragma once

#include "query_utils.h"

#include "query.h"
#include "working_files.h"

#include <optional.h>

Maybe<Reference> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                               QueryFuncId id);
Maybe<Reference> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                               SymbolRef sym);
Maybe<Reference> GetDefinitionExtentOfSymbol(QueryDatabase* db, SymbolRef sym);
Maybe<QueryFileId> GetDeclarationFileForSymbol(QueryDatabase* db,
                                               SymbolRef sym);

std::vector<Use> ToUses(QueryDatabase* db,
                        const std::vector<QueryFuncId>& ids);
std::vector<Use> ToUses(QueryDatabase* db,
                        const std::vector<QueryTypeId>& ids);
std::vector<Use> ToUses(QueryDatabase* db,
                        const std::vector<QueryVarId>& ids);

std::vector<Use> GetUsesOfSymbol(QueryDatabase* db,
                                 SymbolRef sym,
                                 bool include_decl);
std::vector<Use> GetDeclarationsOfSymbolForGotoDefinition(
    QueryDatabase* db,
    SymbolRef sym);

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
                                   Reference location);
std::vector<lsLocation> GetLsLocations(
    QueryDatabase* db,
    WorkingFiles* working_files,
    const std::vector<Use>& refs);
// Returns a symbol. The symbol will have *NOT* have a location assigned.
optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolRef sym,
                                            bool use_short_name);

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile* working_file,
                                             QueryFile* file,
                                             lsPosition position);

void EmitDiagnostics(WorkingFiles* working_files,
                     std::string path,
                     std::vector<lsDiagnostic> diagnostics);

template <typename Q, typename Fn>
void EachWithGen(std::vector<Q>& collection, Id<Q> x, Fn fn) {
  Q& obj = collection[x.id];
  // FIXME Deprecate optional<Def> def
  //  if (obj.gen == x.gen && obj.def)
  if (obj.def)
    fn(obj);
}

template <typename Q, typename Fn>
void EachWithGen(std::vector<Q>& collection, std::vector<Id<Q>>& ids, Fn fn) {
  for (Id<Q> x : ids) {
    Q& obj = collection[x.id];
    if (obj.def) // FIXME Deprecate optional<Def> def
      fn(obj);
  }
}
