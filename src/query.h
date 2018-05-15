#pragma once

#include "indexer.h"
#include "serializer.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>

struct QueryFile;
struct QueryType;
struct QueryFunc;
struct QueryVar;
struct QueryDatabase;

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
    LanguageId language;
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
  int symbol_idx = -1;
  llvm::SmallVector<Def, 1> def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
  std::vector<Usr> derived;
};

struct QueryType : QueryEntity<QueryType, TypeDef> {
  Usr usr;
  int symbol_idx = -1;
  llvm::SmallVector<Def, 1> def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
  std::vector<Usr> derived;
  std::vector<Usr> instances;
};

struct QueryVar : QueryEntity<QueryVar, VarDef> {
  Usr usr;
  int symbol_idx = -1;
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
  std::vector<Usr> funcs_removed;
  std::vector<std::pair<Usr, QueryFunc::Def>> funcs_def_update;
  UseUpdate funcs_declarations;
  UseUpdate funcs_uses;
  UsrUpdate funcs_derived;

  // Type updates.
  std::vector<Usr> types_removed;
  std::vector<std::pair<Usr, QueryType::Def>> types_def_update;
  UseUpdate types_declarations;
  UseUpdate types_uses;
  UsrUpdate types_derived;
  UsrUpdate types_instances;

  // Variable updates.
  std::vector<Usr> vars_removed;
  std::vector<std::pair<Usr, QueryVar::Def>> vars_def_update;
  UseUpdate vars_declarations;
  UseUpdate vars_uses;
};

// The query database is heavily optimized for fast queries. It is stored
// in-memory.
struct QueryDatabase {
  // All File/Func/Type/Var symbols.
  std::vector<SymbolIdx> symbols;

  std::vector<QueryFile> files;
  llvm::StringMap<int> name2file_id;
  std::unordered_map<Usr, QueryFunc> usr2func;
  std::unordered_map<Usr, QueryType> usr2type;
  std::unordered_map<Usr, QueryVar> usr2var;

  // Marks the given Usrs as invalid.
  void RemoveUsrs(SymbolKind usr_kind, const std::vector<Usr>& to_remove);
  void RemoveUsrs(SymbolKind usr_kind, int file_id, const std::vector<Usr>& to_remove);
  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);
  int Update(QueryFile::DefUpdate&& u);
  void Update(int file_id, std::vector<std::pair<Usr, QueryType::Def>>&& us);
  void Update(int file_id, std::vector<std::pair<Usr, QueryFunc::Def>>&& us);
  void Update(int file_id, std::vector<std::pair<Usr, QueryVar::Def>>&& us);
  void UpdateSymbols(int* symbol_idx, SymbolKind kind, Usr usr);
  std::string_view GetSymbolName(int symbol_idx, bool qualified);

  QueryFile& GetFile(SymbolIdx ref) { return files[ref.usr]; }
  QueryFunc& GetFunc(SymbolIdx ref) { return usr2func[ref.usr]; }
  QueryType& GetType(SymbolIdx ref) { return usr2type[ref.usr]; }
  QueryVar& GetVar(SymbolIdx ref) { return usr2var[ref.usr]; }

  QueryFunc& Func(Usr usr) { return usr2func[usr]; }
  QueryType& Type(Usr usr) { return usr2type[usr]; }
  QueryVar& Var(Usr usr) { return usr2var[usr]; }
};
