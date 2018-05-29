#pragma once

#include "clang_tu.h"
#include "clang_utils.h"
#include "file_consumer.h"
#include "language.h"
#include "lsp.h"
#include "maybe.h"
#include "nt_string.h"
#include "position.h"
#include "serializer.h"
#include "symbol.h"
#include "utils.h"

#include <llvm/ADT/StringMap.h>

#include <stdint.h>
#include <algorithm>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

struct IndexFile;
struct IndexType;
struct IndexFunc;
struct IndexVar;
struct QueryFile;

using RawId = uint32_t;

struct SymbolIdx {
  Usr usr;
  SymbolKind kind;

  bool operator==(const SymbolIdx& o) const {
    return usr == o.usr && kind == o.kind;
  }
  bool operator<(const SymbolIdx& o) const {
    return usr != o.usr ? usr < o.usr : kind < o.kind;
  }
};
MAKE_REFLECT_STRUCT(SymbolIdx, usr, kind);

struct Reference {
  Range range;
  Usr usr;
  SymbolKind kind;
  Role role;

  bool Valid() const { return range.Valid(); }
  operator SymbolIdx() const { return {usr, kind}; }
  std::tuple<Range, Usr, SymbolKind, Role> ToTuple() const {
    return std::make_tuple(range, usr, kind, role);
  }
  bool operator==(const Reference& o) const { return ToTuple() == o.ToTuple(); }
  bool operator<(const Reference& o) const { return ToTuple() < o.ToTuple(); }
};

// |id,kind| refer to the referenced entity.
struct SymbolRef : Reference {
  SymbolRef() = default;
  SymbolRef(Range range, Usr usr, SymbolKind kind, Role role)
      : Reference{range, usr, kind, role} {}
};

// Represents an occurrence of a variable/type, |id,kind| refer to the lexical
// parent.
struct Use : Reference {
  // |file| is used in Query* but not in Index*
  int file_id = -1;
};

void Reflect(Reader& visitor, Reference& value);
void Reflect(Writer& visitor, Reference& value);

template <typename D>
struct NameMixin {
  std::string_view Name(bool qualified) const {
    auto self = static_cast<const D*>(this);
    return qualified ? std::string_view(
                           self->detailed_name.c_str() + self->qual_name_offset,
                           self->short_name_offset - self->qual_name_offset +
                               self->short_name_size)
                     : std::string_view(self->detailed_name.c_str() +
                                            self->short_name_offset,
                                        self->short_name_size);
  }
};

struct FuncDef : NameMixin<FuncDef> {
  // General metadata.
  std::string detailed_name;
  NtString hover;
  NtString comments;
  Maybe<Use> spell;
  Maybe<Use> extent;

  // Method this method overrides.
  std::vector<Usr> bases;

  // Local variables or parameters.
  std::vector<Usr> vars;

  // Functions that this function calls.
  std::vector<SymbolRef> callees;

  // Type which declares this one (ie, it is a method)
  Usr declaring_type = 0;
  int16_t qual_name_offset = 0;
  int16_t short_name_offset = 0;
  int16_t short_name_size = 0;
  lsSymbolKind kind = lsSymbolKind::Unknown;
  StorageClass storage = StorageClass::Invalid;

  std::vector<Usr> GetBases() const { return bases; }
  bool operator==(const FuncDef& o) const {
    return detailed_name == o.detailed_name && spell == o.spell &&
           extent == o.extent && declaring_type == o.declaring_type &&
           bases == o.bases && vars == o.vars && callees == o.callees &&
           kind == o.kind && storage == o.storage && hover == o.hover &&
           comments == o.comments;
  }
};
MAKE_REFLECT_STRUCT(FuncDef,
                    detailed_name,
                    qual_name_offset,
                    short_name_offset,
                    short_name_size,
                    kind,
                    storage,
                    hover,
                    comments,
                    spell,
                    extent,
                    declaring_type,
                    bases,
                    vars,
                    callees);

struct IndexFunc : NameMixin<IndexFunc> {
  using Def = FuncDef;
  Usr usr;
  Def def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
  std::vector<Usr> derived;
};

struct TypeDef : NameMixin<TypeDef> {
  // General metadata.
  std::string detailed_name;
  NtString hover;
  NtString comments;

  // While a class/type can technically have a separate declaration/definition,
  // it doesn't really happen in practice. The declaration never contains
  // comments or insightful information. The user always wants to jump from
  // the declaration to the definition - never the other way around like in
  // functions and (less often) variables.
  //
  // It's also difficult to identify a `class Foo;` statement with the clang
  // indexer API (it's doable using cursor AST traversal), so we don't bother
  // supporting the feature.
  Maybe<Use> spell;
  Maybe<Use> extent;

  // Immediate parent types.
  std::vector<Usr> bases;

  // Types, functions, and variables defined in this type.
  std::vector<Usr> types;
  std::vector<Usr> funcs;
  std::vector<std::pair<Usr, int64_t>> vars;

  // If set, then this is the same underlying type as the given value (ie, this
  // type comes from a using or typedef statement).
  Usr alias_of = 0;

  int16_t qual_name_offset = 0;
  int16_t short_name_offset = 0;
  int16_t short_name_size = 0;
  lsSymbolKind kind = lsSymbolKind::Unknown;

  std::vector<Usr> GetBases() const { return bases; }
  bool operator==(const TypeDef& o) const {
    return detailed_name == o.detailed_name && spell == o.spell &&
           extent == o.extent && alias_of == o.alias_of && bases == o.bases &&
           types == o.types && funcs == o.funcs && vars == o.vars &&
           kind == o.kind && hover == o.hover && comments == o.comments;
  }
};
MAKE_REFLECT_STRUCT(TypeDef,
                    detailed_name,
                    qual_name_offset,
                    short_name_offset,
                    short_name_size,
                    kind,
                    hover,
                    comments,
                    spell,
                    extent,
                    alias_of,
                    bases,
                    types,
                    funcs,
                    vars);

struct IndexType {
  using Def = TypeDef;
  Usr usr;
  Def def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
  std::vector<Usr> derived;
  std::vector<Usr> instances;
};

struct VarDef : NameMixin<VarDef> {
  // General metadata.
  std::string detailed_name;
  NtString hover;
  NtString comments;
  // TODO: definitions should be a list of ranges, since there can be more
  //       than one - when??
  Maybe<Use> spell;
  Maybe<Use> extent;

  // Type of the variable.
  Usr type = 0;

  // Function/type which declares this one.
  int16_t qual_name_offset = 0;
  int16_t short_name_offset = 0;
  int16_t short_name_size = 0;

  lsSymbolKind kind = lsSymbolKind::Unknown;
  // Note a variable may have instances of both |None| and |Extern|
  // (declaration).
  StorageClass storage = StorageClass::Invalid;

  bool is_local() const { return kind == lsSymbolKind::Variable; }

  std::vector<Usr> GetBases() const { return {}; }
  bool operator==(const VarDef& o) const {
    return detailed_name == o.detailed_name && spell == o.spell &&
           extent == o.extent && type == o.type && kind == o.kind &&
           storage == o.storage && hover == o.hover && comments == o.comments;
  }
};
MAKE_REFLECT_STRUCT(VarDef,
                    detailed_name,
                    qual_name_offset,
                    short_name_offset,
                    short_name_size,
                    hover,
                    comments,
                    spell,
                    extent,
                    type,
                    kind,
                    storage);

struct IndexVar {
  using Def = VarDef;
  Usr usr;
  Def def;
  std::vector<Use> declarations;
  std::vector<Use> uses;
};

struct IndexInclude {
  // Line that has the include directive. We don't have complete range
  // information - a line is good enough for clicking.
  int line = 0;
  // Absolute path to the index.
  std::string resolved_path;
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
  std::vector<std::string> args;
  int64_t last_write_time = 0;
  LanguageId language = LanguageId::Unknown;

  // The path to the translation unit cc file which caused the creation of this
  // IndexFile. When parsing a translation unit we generate many IndexFile
  // instances (ie, each header has a separate one). When the user edits a
  // header we need to lookup the original translation unit and reindex that.
  std::string import_file;

  // Source ranges that were not processed.
  std::vector<Range> skipped_by_preprocessor;

  std::vector<IndexInclude> includes;
  llvm::StringMap<int64_t> dependencies;
  std::unordered_map<Usr, IndexFunc> usr2func;
  std::unordered_map<Usr, IndexType> usr2type;
  std::unordered_map<Usr, IndexVar> usr2var;

  // Diagnostics found when indexing this file. Not serialized.
  std::vector<lsDiagnostic> diagnostics_;
  // File contents at the time of index. Not serialized.
  std::string file_contents;

  IndexFile(const std::string& path, const std::string& contents);

  IndexFunc& ToFunc(Usr usr);
  IndexType& ToType(Usr usr);
  IndexVar& ToVar(Usr usr);
  IndexFunc& ToFunc(const ClangCursor& c) { return ToFunc(c.get_usr_hash()); }
  IndexType& ToType(const ClangCursor& c) { return ToType(c.get_usr_hash()); }
  IndexVar& ToVar(const ClangCursor& c) { return ToVar(c.get_usr_hash()); }

  std::string ToString();
};

struct NamespaceHelper {
  std::unordered_map<Usr, std::string> usr2qualified_name;

  std::tuple<std::string, int16_t, int16_t> QualifiedName(
      const CXIdxContainerInfo* container,
      std::string_view unqualified_name);
};

std::vector<std::unique_ptr<IndexFile>> ParseWithTu(
    VFS* vfs,
    PerformanceImportFile* perf,
    ClangTranslationUnit* tu,
    ClangIndex* index,
    const std::string& file,
    const std::vector<std::string>& args,
    const std::vector<CXUnsavedFile>& file_contents);

bool ConcatTypeAndName(std::string& type, const std::string& name);

void IndexInit();

struct ClangIndexer {
  std::vector<std::unique_ptr<IndexFile>> Index(
      VFS* vfs,
      std::string file,
      const std::vector<std::string>& args,
      const std::vector<FileContents>& file_contents,
      PerformanceImportFile* perf);

  // Note: constructing this acquires a global lock
  ClangIndex index;
};
