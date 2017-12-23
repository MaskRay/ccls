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
std::vector<QueryLocation> ToQueryLocation(
    QueryDatabase* db,
    const std::vector<QueryFuncRef>& refs);
std::vector<QueryLocation> ToQueryLocation(QueryDatabase* db,
                                           const std::vector<QueryTypeId>& ids);
std::vector<QueryLocation> ToQueryLocation(QueryDatabase* db,
                                           const std::vector<QueryFuncId>& ids);
std::vector<QueryLocation> ToQueryLocation(QueryDatabase* db,
                                           const std::vector<QueryVarId>& ids);
std::vector<QueryLocation> GetUsesOfSymbol(QueryDatabase* db,
                                           const SymbolIdx& symbol);
std::vector<QueryLocation> GetDeclarationsOfSymbolForGotoDefinition(
    QueryDatabase* db,
    const SymbolIdx& symbol);

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
                                   const QueryLocation& location);
std::vector<lsLocation> GetLsLocations(
    QueryDatabase* db,
    WorkingFiles* working_files,
    const std::vector<QueryLocation>& locations);
// Returns a symbol. The symbol will have *NOT* have a location assigned.
optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolIdx symbol,
                                            bool use_short_name);

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile* working_file,
                                             QueryFile* file,
                                             lsPosition position);
