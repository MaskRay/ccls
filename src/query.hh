// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "indexer.hh"
#include "serializer.hh"
#include "working_files.hh"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>

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
  Def *anyDef() {
    Def *ret = nullptr;
    for (auto &i : static_cast<Q *>(this)->def) {
      ret = &i;
      if (i.spell)
        break;
    }
    return ret;
  }
  const Def *anyDef() const {
    return const_cast<QueryEntity *>(this)->anyDef();
  }
};

template <typename T>
using Update =
    std::unordered_map<Usr, std::pair<std::vector<T>, std::vector<T>>>;

struct QueryFunc : QueryEntity<QueryFunc, FuncDef<Vec>> {
  Usr usr;
  llvm::SmallVector<Def, 1> def;
  std::vector<DeclRef> declarations;
  std::vector<Usr> derived;
  std::vector<Use> uses;
};

struct QueryType : QueryEntity<QueryType, TypeDef<Vec>> {
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
  static IndexUpdate createDelta(IndexFile *previous, IndexFile *current);

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
  Update<DeclRef> funcs_declarations;
  Update<Use> funcs_uses;
  Update<Usr> funcs_derived;

  // Type updates.
  int types_hint;
  std::vector<std::pair<Usr, QueryType::Def>> types_removed;
  std::vector<std::pair<Usr, QueryType::Def>> types_def_update;
  Update<DeclRef> types_declarations;
  Update<Use> types_uses;
  Update<Usr> types_derived;
  Update<Usr> types_instances;

  // Variable updates.
  int vars_hint;
  std::vector<std::pair<Usr, QueryVar::Def>> vars_removed;
  std::vector<std::pair<Usr, QueryVar::Def>> vars_def_update;
  Update<DeclRef> vars_declarations;
  Update<Use> vars_uses;
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
  llvm::SmallVector<QueryFunc, 0> funcs;
  llvm::SmallVector<QueryType, 0> types;
  llvm::SmallVector<QueryVar, 0> vars;

  void clear();

  template <typename Def>
  void removeUsrs(Kind kind, int file_id,
                  const std::vector<std::pair<Usr, Def>> &to_remove);
  // Insert the contents of |update| into |db|.
  void applyIndexUpdate(IndexUpdate *update);
  int getFileId(const std::string &path);
  int update(QueryFile::DefUpdate &&u);
  void update(const Lid2file_id &, int file_id,
              std::vector<std::pair<Usr, QueryType::Def>> &&us);
  void update(const Lid2file_id &, int file_id,
              std::vector<std::pair<Usr, QueryFunc::Def>> &&us);
  void update(const Lid2file_id &, int file_id,
              std::vector<std::pair<Usr, QueryVar::Def>> &&us);
  std::string_view getSymbolName(SymbolIdx sym, bool qualified);
  std::vector<uint8_t> getFileSet(const std::vector<std::string> &folders);

  bool hasFunc(Usr usr) const { return func_usr.count(usr); }
  bool hasType(Usr usr) const { return type_usr.count(usr); }
  bool hasVar(Usr usr) const { return var_usr.count(usr); }

  QueryFunc &getFunc(Usr usr) { return funcs[func_usr[usr]]; }
  QueryType &getType(Usr usr) { return types[type_usr[usr]]; }
  QueryVar &getVar(Usr usr) { return vars[var_usr[usr]]; }

  QueryFile &getFile(SymbolIdx ref) { return files[ref.usr]; }
  QueryFunc &getFunc(SymbolIdx ref) { return getFunc(ref.usr); }
  QueryType &getType(SymbolIdx ref) { return getType(ref.usr); }
  QueryVar &getVar(SymbolIdx ref) { return getVar(ref.usr); }
};

Maybe<DeclRef> getDefinitionSpell(DB *db, SymbolIdx sym);

// Get defining declaration (if exists) or an arbitrary declaration (otherwise)
// for each id.
std::vector<Use> getFuncDeclarations(DB *, const std::vector<Usr> &);
std::vector<Use> getFuncDeclarations(DB *, const Vec<Usr> &);
std::vector<Use> getTypeDeclarations(DB *, const std::vector<Usr> &);
std::vector<DeclRef> getVarDeclarations(DB *, const std::vector<Usr> &,
                                        unsigned);

// Get non-defining declarations.
std::vector<DeclRef> &getNonDefDeclarations(DB *db, SymbolIdx sym);

std::vector<Use> getUsesForAllBases(DB *db, QueryFunc &root);
std::vector<Use> getUsesForAllDerived(DB *db, QueryFunc &root);
std::optional<lsRange> getLsRange(WorkingFile *working_file,
                                  const Range &location);
DocumentUri getLsDocumentUri(DB *db, int file_id, std::string *path);
DocumentUri getLsDocumentUri(DB *db, int file_id);

std::optional<Location> getLsLocation(DB *db, WorkingFiles *wfiles, Use use);
std::optional<Location> getLsLocation(DB *db, WorkingFiles *wfiles,
                                      SymbolRef sym, int file_id);
LocationLink getLocationLink(DB *db, WorkingFiles *wfiles, DeclRef dr);

// Returns a symbol. The symbol will *NOT* have a location assigned.
std::optional<SymbolInformation> getSymbolInfo(DB *db, SymbolIdx sym,
                                               bool detailed);

std::vector<SymbolRef> findSymbolsAtLocation(WorkingFile *working_file,
                                             QueryFile *file, Position &ls_pos,
                                             bool smallest = false);

template <typename Fn> void withEntity(DB *db, SymbolIdx sym, Fn &&fn) {
  switch (sym.kind) {
  case Kind::Invalid:
  case Kind::File:
    break;
  case Kind::Func:
    fn(db->getFunc(sym));
    break;
  case Kind::Type:
    fn(db->getType(sym));
    break;
  case Kind::Var:
    fn(db->getVar(sym));
    break;
  }
}

template <typename Fn> void eachEntityDef(DB *db, SymbolIdx sym, Fn &&fn) {
  withEntity(db, sym, [&](const auto &entity) {
    for (auto &def : entity.def)
      if (!fn(def))
        break;
  });
}

template <typename Fn>
void eachOccurrence(DB *db, SymbolIdx sym, bool include_decl, Fn &&fn) {
  withEntity(db, sym, [&](const auto &entity) {
    for (Use use : entity.uses)
      fn(use);
    if (include_decl) {
      for (auto &def : entity.def)
        if (def.spell)
          fn(*def.spell);
      for (Use use : entity.declarations)
        fn(use);
    }
  });
}

SymbolKind getSymbolKind(DB *db, SymbolIdx sym);

template <typename C, typename Fn>
void eachDefinedFunc(DB *db, const C &usrs, Fn &&fn) {
  for (Usr usr : usrs) {
    auto &obj = db->getFunc(usr);
    if (!obj.def.empty())
      fn(obj);
  }
}
} // namespace ccls
