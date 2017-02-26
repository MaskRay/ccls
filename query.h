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
struct UsrRef {
  Usr usr;
  Location loc;

  bool operator==(const UsrRef& other) const {
    return usr == other.usr && loc == other.loc;
  }
};

// There are two sources of reindex updates: the (single) definition of a
// symbol has changed, or one of many users of the symbol has changed.
//
// For simplicitly, if the single definition has changed, we update all of the
// associated single-owner definition data. See |Update*DefId|.
//
// If one of the many symbol users submits an update, we store the update such
// that it can be merged with other updates before actually being applied to
// the main database. See |MergeableUpdate|.

template<typename TValue>
struct MergeableUpdate {
  // The type/func/var which is getting new usages.
  Usr usr;
  // Entries to add and remove.
  std::vector<TValue> to_add;
  std::vector<TValue> to_remove;
};

struct QueryableTypeDef {
  TypeDefDefinitionData<Usr, Usr, Usr> def;
  std::vector<Usr> derived;
  std::vector<Location> uses;

  using DefUpdate = TypeDefDefinitionData<Usr, Usr, Usr>;
  using DerivedUpdate = MergeableUpdate<Usr>;
  using UsesUpdate = MergeableUpdate<Location>;

  QueryableTypeDef(IdCache& id_cache, IndexedTypeDef& indexed);
};

struct QueryableFuncDef {
  FuncDefDefinitionData<Usr, Usr, Usr, UsrRef> def;
  std::vector<Location> declarations;
  std::vector<Usr> derived;
  std::vector<UsrRef> callers;
  std::vector<Location> uses;

  using DefUpdate = FuncDefDefinitionData<Usr, Usr, Usr, UsrRef>;
  using DeclarationsUpdate = MergeableUpdate<Location>;
  using DerivedUpdate = MergeableUpdate<Usr>;
  using CallersUpdate = MergeableUpdate<UsrRef>;
  using UsesUpdate = MergeableUpdate<Location>;

  QueryableFuncDef(IdCache& id_cache, IndexedFuncDef& indexed);
};

struct QueryableVarDef {
  VarDefDefinitionData<Usr, Usr, Usr> def;
  std::vector<Location> uses;

  using DefUpdate = VarDefDefinitionData<Usr, Usr, Usr>;
  using UsesUpdate = MergeableUpdate<Location>;

  QueryableVarDef(IdCache& id_cache, IndexedVarDef& indexed);
};


enum class SymbolKind { Invalid, Type, Func, Var };
struct SymbolIdx {
  SymbolKind kind;
  uint64_t idx;

  SymbolIdx() : kind(SymbolKind::Invalid), idx(-1) {} // Default ctor needed by stdlib. Do not use.
  SymbolIdx(SymbolKind kind, uint64_t idx) : kind(kind), idx(idx) {}
};


struct QueryableFile {
  FileId file_id;

  // Symbols declared in the file.
  std::vector<SymbolIdx> declared_symbols;
  // Symbols which have definitions in the file.
  std::vector<SymbolIdx> defined_symbols;
};