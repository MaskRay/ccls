// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "lsp.hh"
#include "position.hh"
#include "serializer.hh"
#include "utils.hh"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/CachedHashString.h>
#include <llvm/ADT/DenseMap.h>

#include <stdint.h>
#include <string_view>
#include <unordered_map>
#include <vector>

#if LLVM_VERSION_MAJOR >= 19
#define startswith starts_with
#define endswith ends_with
#endif

namespace std {
template <> struct hash<clang::FileID> {
  std::size_t operator()(clang::FileID fid) const { return fid.getHashValue(); }
};
} // namespace std

namespace ccls {
using Usr = uint64_t;

// The order matters. In findSymbolsAtLocation, we want Var/Func ordered in
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
  std::tuple<Range, Usr, Kind, Role> toTuple() const {
    return std::make_tuple(range, usr, kind, role);
  }
  bool operator==(const SymbolRef &o) const { return toTuple() == o.toTuple(); }
  bool valid() const { return range.valid(); }
};

struct ExtentRef : SymbolRef {
  Range extent;
  std::tuple<Range, Usr, Kind, Role, Range> toTuple() const {
    return std::make_tuple(range, usr, kind, role, extent);
  }
  bool operator==(const ExtentRef &o) const { return toTuple() == o.toTuple(); }
};

struct Ref {
  Range range;
  Role role;

  bool valid() const { return range.valid(); }
  std::tuple<Range, Role> toTuple() const {
    return std::make_tuple(range, role);
  }
  bool operator==(const Ref &o) const { return toTuple() == o.toTuple(); }
  bool operator<(const Ref &o) const { return toTuple() < o.toTuple(); }
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

void reflect(JsonReader &visitor, SymbolRef &value);
void reflect(JsonReader &visitor, Use &value);
void reflect(JsonReader &visitor, DeclRef &value);
void reflect(JsonWriter &visitor, SymbolRef &value);
void reflect(JsonWriter &visitor, Use &value);
void reflect(JsonWriter &visitor, DeclRef &value);
void reflect(BinaryReader &visitor, SymbolRef &value);
void reflect(BinaryReader &visitor, Use &value);
void reflect(BinaryReader &visitor, DeclRef &value);
void reflect(BinaryWriter &visitor, SymbolRef &value);
void reflect(BinaryWriter &visitor, Use &value);
void reflect(BinaryWriter &visitor, DeclRef &value);

enum class TokenModifier {
#define TOKEN_MODIFIER(name, str) name,
#include "enum.inc"
#undef TOKEN_MODIFIER
};

template <typename T> using VectorAdapter = std::vector<T, std::allocator<T>>;

template <typename D> struct NameMixin {
  std::string_view name(bool qualified) const {
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

template <template <typename T> class V>
struct FuncDef : NameMixin<FuncDef<V>> {
  // General metadata.
  const char *detailed_name = "";
  const char *hover = "";
  const char *comments = "";
  Maybe<DeclRef> spell;

  // Method this method overrides.
  V<Usr> bases;
  // Local variables or parameters.
  V<Usr> vars;
  // Functions that this function calls.
  V<SymbolRef> callees;

  int file_id = -1; // not serialized
  int16_t qual_name_offset = 0;
  int16_t short_name_offset = 0;
  int16_t short_name_size = 0;
  SymbolKind kind = SymbolKind::Unknown;
  SymbolKind parent_kind = SymbolKind::Unknown;
  uint8_t storage = clang::SC_None;

  const Usr *bases_begin() const { return bases.begin(); }
  const Usr *bases_end() const { return bases.end(); }
};
REFLECT_STRUCT(FuncDef<VectorAdapter>, detailed_name, hover, comments, spell,
               bases, vars, callees, qual_name_offset, short_name_offset,
               short_name_size, kind, parent_kind, storage);

struct IndexFunc : NameMixin<IndexFunc> {
  using Def = FuncDef<VectorAdapter>;
  Usr usr;
  Def def;
  std::vector<DeclRef> declarations;
  std::vector<Usr> derived;
  std::vector<Use> uses;
};

template <template <typename T> class V>
struct TypeDef : NameMixin<TypeDef<V>> {
  const char *detailed_name = "";
  const char *hover = "";
  const char *comments = "";
  Maybe<DeclRef> spell;

  V<Usr> bases;
  // Types, functions, and variables defined in this type.
  V<Usr> funcs;
  V<Usr> types;
  V<std::pair<Usr, int64_t>> vars;

  // If set, then this is the same underlying type as the given value (ie, this
  // type comes from a using or typedef statement).
  Usr alias_of = 0;
  int file_id = -1; // not serialized
  int16_t qual_name_offset = 0;
  int16_t short_name_offset = 0;
  int16_t short_name_size = 0;
  SymbolKind kind = SymbolKind::Unknown;
  SymbolKind parent_kind = SymbolKind::Unknown;

  const Usr *bases_begin() const { return bases.begin(); }
  const Usr *bases_end() const { return bases.end(); }
};
REFLECT_STRUCT(TypeDef<VectorAdapter>, detailed_name, hover, comments, spell,
               bases, funcs, types, vars, alias_of, qual_name_offset,
               short_name_offset, short_name_size, kind, parent_kind);

struct IndexType {
  using Def = TypeDef<VectorAdapter>;
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
           (storage == clang::SC_None || storage == clang::SC_Auto ||
            storage == clang::SC_Register);
  }

  const Usr *bases_begin() const { return nullptr; }
  const Usr *bases_end() const { return nullptr; }
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
  bool no_linkage;

  // uid2lid_and_path is used to generate lid2path, but not serialized.
  std::unordered_map<clang::FileID, std::pair<int, std::string>>
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

  IndexFile(const std::string &path, const std::string &contents,
            bool no_linkage);

  IndexFunc &toFunc(Usr usr);
  IndexType &toType(Usr usr);
  IndexVar &toVar(Usr usr);

  std::string toString();
};

struct IndexResult {
  std::vector<std::unique_ptr<IndexFile>> indexes;
  int n_errs = 0;
  std::string first_error;
};

struct SemaManager;
struct WorkingFiles;
struct VFS;

namespace idx {
void init();
IndexResult
index(SemaManager *complete, WorkingFiles *wfiles, VFS *vfs,
      const std::string &opt_wdir, const std::string &file,
      const std::vector<const char *> &args,
      const std::vector<std::pair<std::string, std::string>> &remapped,
      bool all_linkages, bool &ok);
} // namespace idx
} // namespace ccls

MAKE_HASHABLE(ccls::SymbolRef, t.range, t.usr, t.kind, t.role);
MAKE_HASHABLE(ccls::ExtentRef, t.range, t.usr, t.kind, t.role, t.extent);
MAKE_HASHABLE(ccls::Use, t.range, t.file_id)
MAKE_HASHABLE(ccls::DeclRef, t.range, t.file_id)
