#pragma once

#include "indexer.h"
#include "serializer.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>

struct QueryFile {
  struct Def {
    std::string path;
    std::vector<std::string> args;
    LanguageId language;
    // Includes in the file.
    std::vector<IndexInclude> includes;
    // Outline of the file (ie, for code lens).
    std::vector<SymbolRef> outline;
    // Every symbol found in the file (ie, for goto definition)
    std::vector<SymbolRef> all_symbols;
    // Parts of the file which are disabled.
    std::vector<Range> skipped_ranges;
    // Used by |$ccls/freshenIndex|.
    std::vector<std::string> dependencies;
  };

  using DefUpdate = std::pair<Def, std::string>;

  int id = -1;
  std::optional<Def> def;
};

template <typename Q, typename QDef>
struct QueryEntity {
  using Def = QDef;
  Def* AnyDef() {
    Def* ret = nullptr;
    for (auto& i : static_cast<Q*>(this)->def) {
      ret = &i;
      if (i.spell)
        break;
    }
    return ret;
  }
  const Def* AnyDef() const { return const_cast<QueryEntity*>(this)->AnyDef(); }
};

using UseUpdate =
    std::unordered_map<Usr, std::pair<std::vector<Use>, std::vector<Use>>>;
using UsrUpdate =
    std::unordered_map<Usr, std::pair<std::vector<Usr>, std::vector<Usr>>>;

struct QueryFunc : QueryEntity<QueryFunc, FuncDef> {
  Usr usr;
  llvm::SmallVector<Def, 1> def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
  std::vector<Usr> derived;
};

struct QueryType : QueryEntity<QueryType, TypeDef> {
  Usr usr;
  llvm::SmallVector<Def, 1> def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
  std::vector<Usr> derived;
  std::vector<Usr> instances;
};

struct QueryVar : QueryEntity<QueryVar, VarDef> {
  Usr usr;
  llvm::SmallVector<Def, 1> def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
};

struct IndexUpdate {
  // Creates a new IndexUpdate based on the delta from previous to current. If
  // no delta computation should be done just pass null for previous.
  static IndexUpdate CreateDelta(IndexFile* previous,
                                 IndexFile* current);

  int file_id;

  // Dummy one to refresh all semantic highlight.
  bool refresh = false;

  // File updates.
  std::optional<std::string> files_removed;
  std::optional<QueryFile::DefUpdate> files_def_update;

  // Function updates.
  int funcs_hint;
  std::vector<Usr> funcs_removed;
  std::vector<std::pair<Usr, QueryFunc::Def>> funcs_def_update;
  UseUpdate funcs_declarations;
  UseUpdate funcs_uses;
  UsrUpdate funcs_derived;

  // Type updates.
  int types_hint;
  std::vector<Usr> types_removed;
  std::vector<std::pair<Usr, QueryType::Def>> types_def_update;
  UseUpdate types_declarations;
  UseUpdate types_uses;
  UsrUpdate types_derived;
  UsrUpdate types_instances;

  // Variable updates.
  int vars_hint;
  std::vector<Usr> vars_removed;
  std::vector<std::pair<Usr, QueryVar::Def>> vars_def_update;
  UseUpdate vars_declarations;
  UseUpdate vars_uses;
};

template <typename Q>
struct EntityToIndex {
  using argument_type = const Q&;
  llvm::DenseMap<Usr, unsigned> m;
  unsigned operator()(const Q& entity) const {
    return m[entity.usr];
  }
};

struct WrappedUsr {
  Usr usr;
};
template <>
struct llvm::DenseMapInfo<WrappedUsr> {
  static inline WrappedUsr getEmptyKey() { return {0}; }
  static inline WrappedUsr getTombstoneKey() { return {~0ULL}; }
  static unsigned getHashValue(WrappedUsr w) { return w.usr; }
  static bool isEqual(WrappedUsr l, WrappedUsr r) { return l.usr == r.usr; }
};

// The query database is heavily optimized for fast queries. It is stored
// in-memory.
struct DB {
  std::vector<QueryFile> files;
  llvm::StringMap<int> name2file_id;
  llvm::DenseMap<WrappedUsr, int> func_usr, type_usr, var_usr;
  std::vector<QueryFunc> funcs;
  std::vector<QueryType> types;
  std::vector<QueryVar> vars;

  // Marks the given Usrs as invalid.
  void RemoveUsrs(SymbolKind usr_kind, const std::vector<Usr>& to_remove);
  void RemoveUsrs(SymbolKind usr_kind, int file_id, const std::vector<Usr>& to_remove);
  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);
  int Update(QueryFile::DefUpdate&& u);
  void Update(int file_id, std::vector<std::pair<Usr, QueryType::Def>>&& us);
  void Update(int file_id, std::vector<std::pair<Usr, QueryFunc::Def>>&& us);
  void Update(int file_id, std::vector<std::pair<Usr, QueryVar::Def>>&& us);
  std::string_view GetSymbolName(SymbolIdx sym, bool qualified);

  bool HasFunc(Usr usr) const { return func_usr.count({usr}); }
  bool HasType(Usr usr) const { return type_usr.count({usr}); }
  bool HasVar(Usr usr) const { return var_usr.count({usr}); }

  QueryFunc& Func(Usr usr) { return funcs[func_usr[{usr}]]; }
  QueryType& Type(Usr usr) { return types[type_usr[{usr}]]; }
  QueryVar& Var(Usr usr) { return vars[var_usr[{usr}]]; }

  QueryFile& GetFile(SymbolIdx ref) { return files[ref.usr]; }
  QueryFunc& GetFunc(SymbolIdx ref) { return Func(ref.usr); }
  QueryType& GetType(SymbolIdx ref) { return Type(ref.usr); }
  QueryVar& GetVar(SymbolIdx ref) { return Var(ref.usr); }
};
