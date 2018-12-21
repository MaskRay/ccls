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

#include "lsp.hh"
#include "position.hh"
#include "serializer.hh"
#include "utils.hh"

#include <clang/Basic/FileManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/CachedHashString.h>
#include <llvm/ADT/DenseMap.h>

#include <stdint.h>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace std {
template <> struct hash<llvm::sys::fs::UniqueID> {
  std::size_t operator()(llvm::sys::fs::UniqueID ID) const {
    size_t ret = ID.getDevice();
    ccls::hash_combine(ret, ID.getFile());
    return ret;
  }
};
} // namespace std

namespace ccls {
using Usr = uint64_t;

// The order matters. In FindSymbolsAtLocation, we want Var/Func ordered in
// front of others.
enum class Kind : uint8_t { Invalid, File, Type, Func, Var };
REFLECT_UNDERLYING_B(Kind);

enum class Role : uint16_t {
  None = 0,
  Declaration = 1 << 0,
  Definition = 1 << 1,
  Reference = 1 << 2,
  Read = 1 << 3,
  Write = 1 << 4,
  Call = 1 << 5,
  Dynamic = 1 << 6,
  Address = 1 << 7,
  Implicit = 1 << 8,
  All = (1 << 9) - 1,
};
REFLECT_UNDERLYING_B(Role);
inline uint16_t operator&(Role lhs, Role rhs) {
  return uint16_t(lhs) & uint16_t(rhs);
}
inline Role operator|(Role lhs, Role rhs) {
  return Role(uint16_t(lhs) | uint16_t(rhs));
}

struct SymbolIdx {
  Usr usr;
  Kind kind;

  bool operator==(const SymbolIdx &o) const {
    return usr == o.usr && kind == o.kind;
  }
  bool operator<(const SymbolIdx &o) const {
    return usr != o.usr ? usr < o.usr : kind < o.kind;
  }
};

// |id,kind| refer to the referenced entity.
struct SymbolRef {
  Range range;
  Usr usr;
  Kind kind;
  Role role;
  operator SymbolIdx() const { return {usr, kind}; }
  std::tuple<Range, Usr, Kind, Role> ToTuple() const {
    return std::make_tuple(range, usr, kind, role);
  }
  bool operator==(const SymbolRef &o) const { return ToTuple() == o.ToTuple(); }
  bool Valid() const { return range.Valid(); }
};

struct ExtentRef : SymbolRef {
  Range extent;
  std::tuple<Range, Usr, Kind, Role, Range> ToTuple() const {
    return std::make_tuple(range, usr, kind, role, extent);
  }
  bool operator==(const ExtentRef &o) const { return ToTuple() == o.ToTuple(); }
};

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

struct DeclRef : Use {
  Range extent;
};

void Reflect(JsonReader &visitor, SymbolRef &value);
void Reflect(JsonReader &visitor, Use &value);
void Reflect(JsonReader &visitor, DeclRef &value);
void Reflect(JsonWriter &visitor, SymbolRef &value);
void Reflect(JsonWriter &visitor, Use &value);
void Reflect(JsonWriter &visitor, DeclRef &value);
void Reflect(BinaryReader &visitor, SymbolRef &value);
void Reflect(BinaryReader &visitor, Use &value);
void Reflect(BinaryReader &visitor, DeclRef &value);
void Reflect(BinaryWriter &visitor, SymbolRef &value);
void Reflect(BinaryWriter &visitor, Use &value);
void Reflect(BinaryWriter &visitor, DeclRef &value);

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
  Maybe<DeclRef> spell;

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
  SymbolKind kind = SymbolKind::Unknown;
  SymbolKind parent_kind = SymbolKind::Unknown;
  uint8_t storage = clang::SC_None;

  std::vector<Usr> GetBases() const { return bases; }
};
REFLECT_STRUCT(FuncDef, detailed_name, hover, comments, spell, bases, vars,
                    callees, qual_name_offset, short_name_offset,
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
  Maybe<DeclRef> spell;

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
  SymbolKind kind = SymbolKind::Unknown;
  SymbolKind parent_kind = SymbolKind::Unknown;

  std::vector<Usr> GetBases() const { return bases; }
};
REFLECT_STRUCT(TypeDef, detailed_name, hover, comments, spell, bases,
                    funcs, types, vars, alias_of, qual_name_offset,
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
  Maybe<DeclRef> spell;

  // Type of the variable.
  Usr type = 0;
  int file_id = -1; // not serialized
  int16_t qual_name_offset = 0;
  int16_t short_name_offset = 0;
  int16_t short_name_size = 0;
  SymbolKind kind = SymbolKind::Unknown;
  SymbolKind parent_kind = SymbolKind::Unknown;
  // Note a variable may have instances of both |None| and |Extern|
  // (declaration).
  uint8_t storage = clang::SC_None;

  bool is_local() const {
    return spell &&
           (parent_kind == SymbolKind::Function ||
            parent_kind == SymbolKind::Method ||
            parent_kind == SymbolKind::StaticMethod ||
            parent_kind == SymbolKind::Constructor) &&
           storage == clang::SC_None;
  }

  std::vector<Usr> GetBases() const { return {}; }
};
REFLECT_STRUCT(VarDef, detailed_name, hover, comments, spell, type,
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

struct IndexFile {
  // For both JSON and MessagePack cache files.
  static const int kMajorVersion;
  // For MessagePack cache files.
  // JSON has good forward compatibility because field addition/deletion do not
  // harm but currently no efforts have been made to make old MessagePack cache
  // files accepted by newer ccls.
  static const int kMinorVersion;

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

  IndexFile(const std::string &path, const std::string &contents);

  IndexFunc &ToFunc(Usr usr);
  IndexType &ToType(Usr usr);
  IndexVar &ToVar(Usr usr);

  std::string ToString();
};

struct SemaManager;
struct WorkingFiles;
struct VFS;

namespace idx {
void Init();
std::vector<std::unique_ptr<IndexFile>>
Index(SemaManager *complete, WorkingFiles *wfiles, VFS *vfs,
      const std::string &opt_wdir, const std::string &file,
      const std::vector<const char *> &args,
      const std::vector<std::pair<std::string, std::string>> &remapped,
      bool &ok);
} // namespace idx
} // namespace ccls

MAKE_HASHABLE(ccls::SymbolRef, t.range, t.usr, t.kind, t.role);
MAKE_HASHABLE(ccls::ExtentRef, t.range, t.usr, t.kind, t.role, t.extent);
MAKE_HASHABLE(ccls::Use, t.range, t.file_id)
MAKE_HASHABLE(ccls::DeclRef, t.range, t.file_id)
