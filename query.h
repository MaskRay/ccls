#pragma once

#include "indexer.h"

// NOTE: If updating this enum, make sure to also update the parser and the
//       help text.
enum class Command {
  Callees,
  Callers,
  FindAllUsages,
  FindInterestingUsages,
  GotoReferenced,
  Hierarchy,
  Outline,
  Search
};

// NOTE: If updating this enum, make sure to also update the parser and the
//       help text.
enum class PreferredSymbolLocation {
  Declaration,
  Definition
};

using Usr = std::string;

struct IdMap;

// There are two sources of reindex updates: the (single) definition of a
// symbol has changed, or one of many users of the symbol has changed.
//
// For simplicitly, if the single definition has changed, we update all of the
// associated single-owner definition data. See |Update*DefId|.
//
// If one of the many symbol users submits an update, we store the update such
// that it can be merged with other updates before actually being applied to
// the main database. See |MergeableUpdate|.

template<typename TId, typename TValue>
struct MergeableUpdate {
  // The type/func/var which is getting new usages.
  TId id;
  // Entries to add and remove.
  std::vector<TValue> to_add;
  std::vector<TValue> to_remove;
};


struct QueryableTypeDef {
  TypeDefDefinitionData def;
  std::vector<TypeId> derived;
  std::vector<Location> uses;

  using DefUpdate = TypeDefDefinitionData;
  using DerivedUpdate = MergeableUpdate<TypeId, TypeId>;
  using UsesUpdate = MergeableUpdate<TypeId, Location>;

  QueryableTypeDef(IdMap& id_map, const IndexedTypeDef& indexed);
};

struct QueryableFuncDef {
  FuncDefDefinitionData def;
  std::vector<Location> declarations;
  std::vector<FuncId> derived;
  std::vector<FuncRef> callers;
  std::vector<Location> uses;

  using DefUpdate = FuncDefDefinitionData;
  using DeclarationsUpdate = MergeableUpdate<FuncId, Location>;
  using DerivedUpdate = MergeableUpdate<FuncId, FuncId>;
  using CallersUpdate = MergeableUpdate<FuncId, FuncRef>;
  using UsesUpdate = MergeableUpdate<FuncId, Location>;

  QueryableFuncDef(IdMap& id_map, const IndexedFuncDef& indexed);
};

struct QueryableVarDef {
  VarDefDefinitionData def;
  std::vector<Location> uses;

  using DefUpdate = VarDefDefinitionData;
  using UsesUpdate = MergeableUpdate<VarId, Location>;

  QueryableVarDef(IdMap& id_map, const IndexedVarDef& indexed);
};


enum class SymbolKind { Type, Func, Var };
struct SymbolIdx {
  SymbolKind kind;
  union {
    uint64_t type_idx;
    uint64_t func_idx;
    uint64_t var_idx;
  };
};


struct QueryableFile {
  FileId file_id;

  // Symbols declared in the file.
  std::vector<SymbolIdx> declared_symbols;
  // Symbols which have definitions in the file.
  std::vector<SymbolIdx> defined_symbols;
};