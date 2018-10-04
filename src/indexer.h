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

#include "language.h"
#include "lsp.h"
#include "lsp_diagnostic.h"
#include "maybe.h"
#include "position.h"
#include "serializer.h"
#include "symbol.h"
#include "utils.h"

#include <clang/Basic/FileManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/CachedHashString.h>
#include <llvm/ADT/DenseMap.h>

#include <stdint.h>
#include <string_view>
#include <unordered_map>
#include <vector>

using Usr = uint64_t;

struct SymbolIdx {
  Usr usr;
  SymbolKind kind;

  bool operator==(const SymbolIdx &o) const {
    return usr == o.usr && kind == o.kind;
  }
  bool operator<(const SymbolIdx &o) const {
    return usr != o.usr ? usr < o.usr : kind < o.kind;
  }
};
MAKE_REFLECT_STRUCT(SymbolIdx, usr, kind);

// |id,kind| refer to the referenced entity.
struct SymbolRef {
  Range range;
  Usr usr;
  SymbolKind kind;
  Role role;
  operator SymbolIdx() const { return {usr, kind}; }
  std::tuple<Range, Usr, SymbolKind, Role> ToTuple() const {
    return std::make_tuple(range, usr, kind, role);
  }
  bool operator==(const SymbolRef &o) const { return ToTuple() == o.ToTuple(); }
  bool Valid() const { return range.Valid(); }
};
MAKE_HASHABLE(SymbolRef, t.range, t.usr, t.kind, t.role);

struct Ref {
  Range range;
  Role role;

  bool Valid() const { return range.Valid(); }
  std::tuple<Range, Role> ToTuple() const {
    return std::make_tuple(range, role);
  }
  bool operator==(const Ref &o) const { return ToTuple() == o.ToTuple(); }
  bool operator<(const Ref &o) const { return ToTuple() < o.ToTuple(); }
};

// Represents an occurrence of a variable/type, |usr,kind| refer to the lexical
// parent.
struct Use : Ref {
  // |file| is used in Query* but not in Index*
  int file_id = -1;
  bool operator==(const Use &o) const {
    // lexical container info is ignored.
    return range == o.range && file_id == o.file_id;
  }
};
MAKE_HASHABLE(Use, t.range, t.file_id)

struct DeclRef : Use {
  Range extent;
};
MAKE_HASHABLE(DeclRef, t.range, t.file_id)

void Reflect(Reader &visitor, SymbolRef &value);
void Reflect(Writer &visitor, SymbolRef &value);
void Reflect(Reader &visitor, Use &value);
void Reflect(Writer &visitor, Use &value);
void Reflect(Reader &visitor, DeclRef &value);
void Reflect(Writer &visitor, DeclRef &value);

template <typename D> struct NameMixin {
  std::string_view Name(bool qualified) const {
    auto self = static_cast<const D *>(this);
    return qualified
               ? std::string_view(self->detailed_name + self->qual_name_offset,
                                  self->short_name_offset -
                                      self->qual_name_offset +
                                      self->short_name_size)
               : std::string_view(self->detailed_name + self->short_name_offset,
                                  self->short_name_size);
  }
};

struct FuncDef : NameMixin<FuncDef> {
  // General metadata.
  const char *detailed_name = "";
  const char *hover = "";
  const char *comments = "";
  Maybe<Use> spell;
  Maybe<Use> extent;

  // Method this method overrides.
  std::vector<Usr> bases;
  // Local variables or parameters.
  std::vector<Usr> vars;
  // Functions that this function calls.
  std::vector<SymbolRef> callees;

  int file_id = -1; // not serialized
  int16_t qual_name_offset = 0;
  int16_t short_name_offset = 0;
  int16_t short_name_size = 0;
  lsSymbolKind kind = lsSymbolKind::Unknown;
  lsSymbolKind parent_kind = lsSymbolKind::Unknown;
  uint8_t storage = clang::SC_None;

  std::vector<Usr> GetBases() const { return bases; }
};
MAKE_REFLECT_STRUCT(FuncDef, detailed_name, hover, comments, spell, extent,
                    bases, vars, callees, qual_name_offset, short_name_offset,
                    short_name_size, kind, parent_kind, storage);

struct IndexFunc : NameMixin<IndexFunc> {
  using Def = FuncDef;
  Usr usr;
  Def def;
  std::vector<DeclRef> declarations;
  std::vector<Usr> derived;
  std::vector<Use> uses;
};

struct TypeDef : NameMixin<TypeDef> {
  const char *detailed_name = "";
  const char *hover = "";
  const char *comments = "";
  Maybe<Use> spell;
  Maybe<Use> extent;

  std::vector<Usr> bases;
  // Types, functions, and variables defined in this type.
  std::vector<Usr> funcs;
  std::vector<Usr> types;
  std::vector<std::pair<Usr, int64_t>> vars;

  // If set, then this is the same underlying type as the given value (ie, this
  // type comes from a using or typedef statement).
  Usr alias_of = 0;
  int file_id = -1; // not serialized
  int16_t qual_name_offset = 0;
  int16_t short_name_offset = 0;
  int16_t short_name_size = 0;
  lsSymbolKind kind = lsSymbolKind::Unknown;
  lsSymbolKind parent_kind = lsSymbolKind::Unknown;

  std::vector<Usr> GetBases() const { return bases; }
};
MAKE_REFLECT_STRUCT(TypeDef, detailed_name, hover, comments, spell, extent,
                    bases, funcs, types, vars, alias_of, qual_name_offset,
                    short_name_offset, short_name_size, kind, parent_kind);

struct IndexType {
  using Def = TypeDef;
  Usr usr;
  Def def;
  std::vector<DeclRef> declarations;
  std::vector<Usr> derived;
  std::vector<Usr> instances;
  std::vector<Use> uses;
};

struct VarDef : NameMixin<VarDef> {
  // General metadata.
  const char *detailed_name = "";
  const char *hover = "";
  const char *comments = "";
  Maybe<Use> spell;
  Maybe<Use> extent;

  // Type of the variable.
  Usr type = 0;
  int file_id = -1; // not serialized
  int16_t qual_name_offset = 0;
  int16_t short_name_offset = 0;
  int16_t short_name_size = 0;
  lsSymbolKind kind = lsSymbolKind::Unknown;
  lsSymbolKind parent_kind = lsSymbolKind::Unknown;
  // Note a variable may have instances of both |None| and |Extern|
  // (declaration).
  uint8_t storage = clang::SC_None;

  bool is_local() const {
    return spell &&
           (parent_kind == lsSymbolKind::Function ||
            parent_kind == lsSymbolKind::Method ||
            parent_kind == lsSymbolKind::StaticMethod) &&
           storage == clang::SC_None;
  }

  std::vector<Usr> GetBases() const { return {}; }
};
MAKE_REFLECT_STRUCT(VarDef, detailed_name, hover, comments, spell, extent, type,
                    qual_name_offset, short_name_offset, short_name_size, kind,
                    parent_kind, storage);

struct IndexVar {
  using Def = VarDef;
  Usr usr;
  Def def;
  std::vector<DeclRef> declarations;
  std::vector<Use> uses;
};

struct IndexInclude {
  // Line that has the include directive. We don't have complete range
  // information - a line is good enough for clicking.
  int line = 0;
  // Absolute path to the index.
  const char *resolved_path;
};

namespace std {
template <> struct hash<llvm::sys::fs::UniqueID> {
  std::size_t operator()(llvm::sys::fs::UniqueID ID) const {
    size_t ret = ID.getDevice();
    hash_combine(ret, ID.getFile());
    return ret;
  }
};
} // namespace std

struct IndexFile {
  // For both JSON and MessagePack cache files.
  static const int kMajorVersion;
  // For MessagePack cache files.
  // JSON has good forward compatibility because field addition/deletion do not
  // harm but currently no efforts have been made to make old MessagePack cache
  // files accepted by newer ccls.
  static const int kMinorVersion;

  llvm::sys::fs::UniqueID UniqueID;
  std::string path;
  std::vector<const char *> args;
  // This is unfortunately time_t as used by clang::FileEntry
  int64_t mtime = 0;
  LanguageId language = LanguageId::C;

  // uid2lid_and_path is used to generate lid2path, but not serialized.
  std::unordered_map<llvm::sys::fs::UniqueID, std::pair<int, std::string>>
      uid2lid_and_path;
  std::vector<std::pair<int, std::string>> lid2path;

  // The path to the translation unit cc file which caused the creation of this
  // IndexFile. When parsing a translation unit we generate many IndexFile
  // instances (ie, each header has a separate one). When the user edits a
  // header we need to lookup the original translation unit and reindex that.
  std::string import_file;

  // Source ranges that were not processed.
  std::vector<Range> skipped_ranges;

  std::vector<IndexInclude> includes;
  llvm::DenseMap<llvm::CachedHashStringRef, int64_t> dependencies;
  std::unordered_map<Usr, IndexFunc> usr2func;
  std::unordered_map<Usr, IndexType> usr2type;
  std::unordered_map<Usr, IndexVar> usr2var;

  // File contents at the time of index. Not serialized.
  std::string file_contents;

  IndexFile(llvm::sys::fs::UniqueID UniqueID, const std::string &path,
            const std::string &contents);

  IndexFunc &ToFunc(Usr usr);
  IndexType &ToType(Usr usr);
  IndexVar &ToVar(Usr usr);

  std::string ToString();
};

struct CompletionManager;
struct WorkingFiles;
struct VFS;

namespace ccls::idx {
void Init();
std::vector<std::unique_ptr<IndexFile>>
Index(CompletionManager *complete, WorkingFiles *wfiles, VFS *vfs,
      const std::string &opt_wdir, const std::string &file,
      const std::vector<const char *> &args,
      const std::vector<std::pair<std::string, std::string>> &remapped,
      bool &ok);
} // namespace ccls::idx
