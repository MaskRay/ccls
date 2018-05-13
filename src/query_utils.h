#pragma once

#include "query.h"
#include "working_files.h"

#include <optional>

Maybe<Use> GetDefinitionSpell(QueryDatabase* db, SymbolIdx sym);
Maybe<Use> GetDefinitionExtent(QueryDatabase* db, SymbolIdx sym);

// Get defining declaration (if exists) or an arbitrary declaration (otherwise)
// for each id.
template <typename Q>
std::vector<Use> GetDeclarations(llvm::DenseMap<Usr, Q>& usr2entity,
                                 const std::vector<Usr>& usrs) {
  std::vector<Use> ret;
  ret.reserve(usrs.size());
  for (Usr usr : usrs) {
    Q& entity = usr2entity[usr];
    bool has_def = false;
    for (auto& def : entity.def)
      if (def.spell) {
        ret.push_back(*def.spell);
        has_def = true;
        break;
      }
    if (!has_def && entity.declarations.size())
      ret.push_back(entity.declarations[0]);
  }
  return ret;
}

// Get non-defining declarations.
std::vector<Use> GetNonDefDeclarations(QueryDatabase* db, SymbolIdx sym);

std::vector<Use> GetUsesForAllBases(QueryDatabase* db, QueryFunc& root);
std::vector<Use> GetUsesForAllDerived(QueryDatabase* db, QueryFunc& root);
std::optional<lsPosition> GetLsPosition(WorkingFile* working_file,
                                   const Position& position);
std::optional<lsRange> GetLsRange(WorkingFile* working_file, const Range& location);
lsDocumentUri GetLsDocumentUri(QueryDatabase* db,
                               int file_id,
                               std::string* path);
lsDocumentUri GetLsDocumentUri(QueryDatabase* db, int file_id);

std::optional<lsLocation> GetLsLocation(QueryDatabase* db,
                                   WorkingFiles* working_files,
                                   Use use);
std::optional<lsLocationEx> GetLsLocationEx(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       Use use,
                                       bool container);
std::vector<lsLocationEx> GetLsLocationExs(QueryDatabase* db,
                                           WorkingFiles* working_files,
                                           const std::vector<Use>& refs);
// Returns a symbol. The symbol will have *NOT* have a location assigned.
std::optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolIdx sym,
                                            bool detailed_name);

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile* working_file,
                                             QueryFile* file,
                                             lsPosition& ls_pos);

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
void EachOccurrence(QueryDatabase* db,
                    SymbolIdx sym,
                    bool include_decl,
                    Fn&& fn) {
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

lsSymbolKind GetSymbolKind(QueryDatabase* db, SymbolIdx sym);

template <typename Fn>
void EachOccurrenceWithParent(QueryDatabase* db,
                              SymbolIdx sym,
                              bool include_decl,
                              Fn&& fn) {
  WithEntity(db, sym, [&](const auto& entity) {
    lsSymbolKind parent_kind = lsSymbolKind::Unknown;
    for (auto& def : entity.def)
      if (def.spell) {
        parent_kind = GetSymbolKind(db, sym);
        break;
      }
    for (Use use : entity.uses)
      fn(use, parent_kind);
    if (include_decl) {
      for (auto& def : entity.def)
        if (def.spell)
          fn(*def.spell, parent_kind);
      for (Use use : entity.declarations)
        fn(use, parent_kind);
    }
  });
}

template <typename Q, typename Fn>
void EachDefinedEntity(llvm::DenseMap<Usr, Q>& collection,
                       const std::vector<Usr>& usrs,
                       Fn&& fn) {
  for (Usr usr : usrs) {
    Q& obj = collection[usr];
    if (!obj.def.empty())
      fn(obj);
  }
}
