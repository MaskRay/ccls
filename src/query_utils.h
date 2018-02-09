#pragma once

#include "query_utils.h"

#include "query.h"
#include "working_files.h"

#include <optional.h>

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const QueryTypeId& id);
optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const QueryFuncId& id);
optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const QueryVarId& id);
optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const SymbolIdx& symbol);
optional<QueryLocation> GetDefinitionExtentOfSymbol(QueryDatabase* db,
                                                    const SymbolIdx& symbol);
optional<QueryFileId> GetDeclarationFileForSymbol(QueryDatabase* db,
                                                  const SymbolIdx& symbol);

QueryFileId GetFileId(QueryDatabase* db, Reference ref);

std::vector<Reference> ToReference(QueryDatabase* db,
                                   const std::vector<QueryFuncRef>& refs);

template <typename Q>
std::vector<Reference> ToReference(QueryDatabase* db,
                                   const std::vector<Id<Q>>& ids) {
  std::vector<Reference> ret;
  ret.reserve(ids.size());
  for (auto id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      ret.push_back(*loc);
  }
  return ret;
}

std::vector<Reference> GetUsesOfSymbol(QueryDatabase* db,
                                       const SymbolIdx& symbol,
                                       bool include_decl);
std::vector<Reference> GetDeclarationsOfSymbolForGotoDefinition(
    QueryDatabase* db,
    SymbolIdx symbol);

bool HasCallersOnSelfOrBaseOrDerived(QueryDatabase* db, QueryFunc& root);
std::vector<QueryFuncRef> GetCallersForAllBaseFunctions(QueryDatabase* db,
                                                        QueryFunc& root);
std::vector<QueryFuncRef> GetCallersForAllDerivedFunctions(QueryDatabase* db,
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
    const std::vector<Reference>& refs);
// Returns a symbol. The symbol will have *NOT* have a location assigned.
optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolIdx symbol,
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
