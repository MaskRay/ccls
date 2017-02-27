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

// TODO: Switch over to QueryableLocation. Figure out if there is
//       a good way to get the indexer using it. I don't think so
//       since we may discover more files while indexing a file.
//
//       We could also reuse planned USR caching system for file
//       paths.
struct QueryableLocation {
  Usr path;
  int line;
  int column;
  bool interesting;

  QueryableLocation()
    : path(""), line(-1), column(-1), interesting(false) {}
  QueryableLocation(Usr path, int line, int column, bool interesting)
    : path(path), line(line), column(column), interesting(interesting) {}

  bool operator==(const QueryableLocation& other) const {
    // Note: We ignore |is_interesting|.
    return
      path == other.path &&
      line == other.line &&
      column == other.column;
  }
};


struct UsrRef {
  Usr usr;
  QueryableLocation loc;

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
  using DefUpdate = TypeDefDefinitionData<Usr, Usr, Usr, QueryableLocation>;
  using DerivedUpdate = MergeableUpdate<Usr>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<Usr> derived;
  std::vector<QueryableLocation> uses;

  QueryableTypeDef(IdCache& id_cache, IndexedTypeDef& indexed);
};

struct QueryableFuncDef {
  using DefUpdate = FuncDefDefinitionData<Usr, Usr, Usr, UsrRef, QueryableLocation>;
  using DeclarationsUpdate = MergeableUpdate<QueryableLocation>;
  using DerivedUpdate = MergeableUpdate<Usr>;
  using CallersUpdate = MergeableUpdate<UsrRef>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<QueryableLocation> declarations;
  std::vector<Usr> derived;
  std::vector<UsrRef> callers;
  std::vector<QueryableLocation> uses;

  QueryableFuncDef(IdCache& id_cache, IndexedFuncDef& indexed);
};

struct QueryableVarDef {
  using DefUpdate = VarDefDefinitionData<Usr, Usr, Usr, QueryableLocation>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<QueryableLocation> uses;

  QueryableVarDef(IdCache& id_cache, IndexedVarDef& indexed);
};


enum class SymbolKind { Invalid, File, Type, Func, Var };
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