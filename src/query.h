#pragma once

#include "indexer.h"
#include "serializer.h"

#include <sparsepp/spp.h>

#include <forward_list>

struct QueryFile;
struct QueryType;
struct QueryFunc;
struct QueryVar;
struct QueryDatabase;

using QueryFileId = Id<QueryFile>;
using QueryTypeId = Id<QueryType>;
using QueryFuncId = Id<QueryFunc>;
using QueryVarId = Id<QueryVar>;

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

template <typename TValue>
struct MergeableUpdate {
  // The type/func/var which is getting new usages.
  Usr usr;
  // Entries to add and remove.
  std::vector<TValue> to_remove;
  std::vector<TValue> to_add;

  MergeableUpdate(Usr usr,
                  std::vector<TValue>&& to_remove,
                  std::vector<TValue>&& to_add)
      : usr(usr), to_remove(std::move(to_remove)), to_add(std::move(to_add)) {}
  MergeableUpdate(Usr usr,
                  const std::vector<TValue>& to_remove,
                  const std::vector<TValue>& to_add)
      : usr(usr), to_remove(to_remove), to_add(to_add) {}
};

template <typename T>
struct WithUsr {
  Usr usr;
  T value;

  WithUsr(Usr usr, const T& value) : usr(usr), value(value) {}
  WithUsr(Usr usr, T&& value) : usr(usr), value(value) {}
};

template <typename T>
struct WithFileContent {
  T value;
  std::string file_content;

  WithFileContent(const T& value, const std::string& file_content)
      : value(value), file_content(file_content) {}
};

struct QueryFile {
  struct Def {
    std::string path;
    std::vector<std::string> args;
    // Language identifier
    std::string language;
    // Includes in the file.
    std::vector<IndexInclude> includes;
    // Outline of the file (ie, for code lens).
    std::vector<SymbolRef> outline;
    // Every symbol found in the file (ie, for goto definition)
    std::vector<SymbolRef> all_symbols;
    // Parts of the file which are disabled.
    std::vector<Range> inactive_regions;
    // Used by |$ccls/freshenIndex|.
    std::vector<std::string> dependencies;
  };

  using DefUpdate = WithFileContent<Def>;

  int id = -1;
  std::optional<Def> def;
  int symbol_idx = -1;
};

template <typename Q, typename QDef>
struct QueryEntity {
  using Def = QDef;
  using DefUpdate = WithUsr<Def>;
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

using UsrUpdate = MergeableUpdate<Usr>;
using UseUpdate = MergeableUpdate<Use>;

struct QueryFunc : QueryEntity<QueryFunc, FuncDef> {
  Usr usr;
  int symbol_idx = -1;
  std::forward_list<Def> def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
  std::vector<Usr> derived;
};

struct QueryType : QueryEntity<QueryType, TypeDef> {
  Usr usr;
  int symbol_idx = -1;
  std::forward_list<Def> def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
  std::vector<Usr> derived;
  std::vector<Usr> instances;
};

struct QueryVar : QueryEntity<QueryVar, VarDef> {
  Usr usr;
  int symbol_idx = -1;
  std::forward_list<Def> def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
};

struct IndexUpdate {
  // Creates a new IndexUpdate based on the delta from previous to current. If
  // no delta computation should be done just pass null for previous.
  static IndexUpdate CreateDelta(IndexFile* previous,
                                 IndexFile* current);

  // Merge |update| into this update; this can reduce overhead / index update
  // work can be parallelized.
  void Merge(IndexUpdate&& update);

  // Dump the update to a string.
  std::string ToString();

  int file_id;

  // File updates.
  std::optional<std::string> files_removed;
  std::optional<QueryFile::DefUpdate> files_def_update;

  // Function updates.
  std::vector<Usr> funcs_removed;
  std::vector<QueryFunc::DefUpdate> funcs_def_update;
  std::vector<UseUpdate> funcs_declarations;
  std::vector<UseUpdate> funcs_uses;
  std::vector<UsrUpdate> funcs_derived;

  // Type updates.
  std::vector<Usr> types_removed;
  std::vector<QueryType::DefUpdate> types_def_update;
  std::vector<UseUpdate> types_declarations;
  std::vector<UseUpdate> types_uses;
  std::vector<UsrUpdate> types_derived;
  std::vector<UsrUpdate> types_instances;

  // Variable updates.
  std::vector<Usr> vars_removed;
  std::vector<QueryVar::DefUpdate> vars_def_update;
  std::vector<UseUpdate> vars_declarations;
  std::vector<UseUpdate> vars_uses;
};

// The query database is heavily optimized for fast queries. It is stored
// in-memory.
struct QueryDatabase {
  // All File/Func/Type/Var symbols.
  std::vector<SymbolIdx> symbols;

  std::vector<QueryFile> files;
  std::unordered_map<std::string, int> name2file_id;
  spp::sparse_hash_map<Usr, QueryFunc> usr2func;
  spp::sparse_hash_map<Usr, QueryType> usr2type;
  spp::sparse_hash_map<Usr, QueryVar> usr2var;

  // Marks the given Usrs as invalid.
  void RemoveUsrs(SymbolKind usr_kind, const std::vector<Usr>& to_remove);
  void RemoveUsrs(SymbolKind usr_kind, int file_id, const std::vector<Usr>& to_remove);
  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);
  int Update(QueryFile::DefUpdate&& u);
  void Update(int file_id, std::vector<QueryType::DefUpdate>&& updates);
  void Update(int file_id, std::vector<QueryFunc::DefUpdate>&& updates);
  void Update(int file_id, std::vector<QueryVar::DefUpdate>&& updates);
  void UpdateSymbols(int* symbol_idx,
                     SymbolKind kind,
                     Usr usr);
  std::string_view GetSymbolName(int symbol_idx, bool qualified);

  QueryFile& GetFile(SymbolIdx ref) { return files[ref.usr]; }
  QueryFunc& GetFunc(SymbolIdx ref) { return usr2func[ref.usr]; }
  QueryType& GetType(SymbolIdx ref) { return usr2type[ref.usr]; }
  QueryVar& GetVar(SymbolIdx ref) { return usr2var[ref.usr]; }

  QueryFunc& Func(Usr usr) { return usr2func[usr]; }
  QueryType& Type(Usr usr) { return usr2type[usr]; }
  QueryVar& Var(Usr usr) { return usr2var[usr]; }
};
