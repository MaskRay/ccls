/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once

#include "indexer.hh"
#include "serializer.hh"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>

namespace llvm {
template <> struct DenseMapInfo<ccls::ExtentRef> {
  static inline ccls::ExtentRef getEmptyKey() { return {}; }
  static inline ccls::ExtentRef getTombstoneKey() {
    return {{ccls::Range(), ccls::Usr(-1)}};
  }
  static unsigned getHashValue(ccls::ExtentRef sym) {
    return std::hash<ccls::ExtentRef>()(sym);
  }
  static bool isEqual(ccls::ExtentRef l, ccls::ExtentRef r) { return l == r; }
};
} // namespace llvm

namespace ccls {
struct QueryFile {
  struct Def {
    std::string path;
    std::vector<const char *> args;
    LanguageId language;
    // Includes in the file.
    std::vector<IndexInclude> includes;
    // Parts of the file which are disabled.
    std::vector<Range> skipped_ranges;
    // Used by |$ccls/reload|.
    std::vector<const char *> dependencies;
  };

  using DefUpdate = std::pair<Def, std::string>;

  int id = -1;
  std::optional<Def> def;
  // `extent` is valid => declaration; invalid => regular reference
  llvm::DenseMap<ExtentRef, int> symbol2refcnt;
};

template <typename Q, typename QDef> struct QueryEntity {
  using Def = QDef;
  Def *AnyDef() {
    Def *ret = nullptr;
    for (auto &i : static_cast<Q *>(this)->def) {
      ret = &i;
      if (i.spell)
        break;
    }
    return ret;
  }
  const Def *AnyDef() const {
    return const_cast<QueryEntity *>(this)->AnyDef();
  }
};

using DeclRefUpdate =
    std::unordered_map<Usr,
                       std::pair<std::vector<DeclRef>, std::vector<DeclRef>>>;
using UseUpdate =
    std::unordered_map<Usr, std::pair<std::vector<Use>, std::vector<Use>>>;
using UsrUpdate =
    std::unordered_map<Usr, std::pair<std::vector<Usr>, std::vector<Usr>>>;

struct QueryFunc : QueryEntity<QueryFunc, FuncDef> {
  Usr usr;
  llvm::SmallVector<Def, 1> def;
  std::vector<DeclRef> declarations;
  std::vector<Usr> derived;
  std::vector<Use> uses;
};

struct QueryType : QueryEntity<QueryType, TypeDef> {
  Usr usr;
  llvm::SmallVector<Def, 1> def;
  std::vector<DeclRef> declarations;
  std::vector<Usr> derived;
  std::vector<Usr> instances;
  std::vector<Use> uses;
};

struct QueryVar : QueryEntity<QueryVar, VarDef> {
  Usr usr;
  llvm::SmallVector<Def, 1> def;
  std::vector<DeclRef> declarations;
  std::vector<Use> uses;
};

struct IndexUpdate {
  // Creates a new IndexUpdate based on the delta from previous to current. If
  // no delta computation should be done just pass null for previous.
  static IndexUpdate CreateDelta(IndexFile *previous, IndexFile *current);

  int file_id;

  // Dummy one to refresh all semantic highlight.
  bool refresh = false;

  decltype(IndexFile::lid2path) prev_lid2path;
  decltype(IndexFile::lid2path) lid2path;

  // File updates.
  std::optional<std::string> files_removed;
  std::optional<QueryFile::DefUpdate> files_def_update;

  // Function updates.
  int funcs_hint;
  std::vector<std::pair<Usr, QueryFunc::Def>> funcs_removed;
  std::vector<std::pair<Usr, QueryFunc::Def>> funcs_def_update;
  DeclRefUpdate funcs_declarations;
  UseUpdate funcs_uses;
  UsrUpdate funcs_derived;

  // Type updates.
  int types_hint;
  std::vector<std::pair<Usr, QueryType::Def>> types_removed;
  std::vector<std::pair<Usr, QueryType::Def>> types_def_update;
  DeclRefUpdate types_declarations;
  UseUpdate types_uses;
  UsrUpdate types_derived;
  UsrUpdate types_instances;

  // Variable updates.
  int vars_hint;
  std::vector<std::pair<Usr, QueryVar::Def>> vars_removed;
  std::vector<std::pair<Usr, QueryVar::Def>> vars_def_update;
  DeclRefUpdate vars_declarations;
  UseUpdate vars_uses;
};

struct DenseMapInfoForUsr {
  static inline Usr getEmptyKey() { return 0; }
  static inline Usr getTombstoneKey() { return ~0ULL; }
  static unsigned getHashValue(Usr w) { return w; }
  static bool isEqual(Usr l, Usr r) { return l == r; }
};

using Lid2file_id = std::unordered_map<int, int>;

// The query database is heavily optimized for fast queries. It is stored
// in-memory.
struct DB {
  std::vector<QueryFile> files;
  llvm::StringMap<int> name2file_id;
  llvm::DenseMap<Usr, int, DenseMapInfoForUsr> func_usr, type_usr, var_usr;
  std::vector<QueryFunc> funcs;
  std::vector<QueryType> types;
  std::vector<QueryVar> vars;

  void clear();

  template <typename Def>
  void RemoveUsrs(Kind kind, int file_id,
                  const std::vector<std::pair<Usr, Def>> &to_remove);
  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate *update);
  int GetFileId(const std::string &path);
  int Update(QueryFile::DefUpdate &&u);
  void Update(const Lid2file_id &, int file_id,
              std::vector<std::pair<Usr, QueryType::Def>> &&us);
  void Update(const Lid2file_id &, int file_id,
              std::vector<std::pair<Usr, QueryFunc::Def>> &&us);
  void Update(const Lid2file_id &, int file_id,
              std::vector<std::pair<Usr, QueryVar::Def>> &&us);
  std::string_view GetSymbolName(SymbolIdx sym, bool qualified);
  std::vector<uint8_t> GetFileSet(const std::vector<std::string> &folders);

  bool HasFunc(Usr usr) const { return func_usr.count(usr); }
  bool HasType(Usr usr) const { return type_usr.count(usr); }
  bool HasVar(Usr usr) const { return var_usr.count(usr); }

  QueryFunc &Func(Usr usr) { return funcs[func_usr[usr]]; }
  QueryType &Type(Usr usr) { return types[type_usr[usr]]; }
  QueryVar &Var(Usr usr) { return vars[var_usr[usr]]; }

  QueryFile &GetFile(SymbolIdx ref) { return files[ref.usr]; }
  QueryFunc &GetFunc(SymbolIdx ref) { return Func(ref.usr); }
  QueryType &GetType(SymbolIdx ref) { return Type(ref.usr); }
  QueryVar &GetVar(SymbolIdx ref) { return Var(ref.usr); }
};
} // namespace ccls
