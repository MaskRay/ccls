#pragma once

#include "clang_index.h"
#include "clang_translation_unit.h"
#include "clang_utils.h"
#include "file_consumer.h"
#include "language_server_api.h"
#include "performance.h"
#include "position.h"
#include "serializer.h"
#include "utils.h"

#include <optional.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

struct IndexType;
struct IndexFunc;
struct IndexVar;

using namespace std::experimental;

template <typename T>
struct Id {
  size_t id;

  Id() : id(0) {}  // Needed for containers. Do not use directly.
  explicit Id(size_t id) : id(id) {}

  // Needed for google::dense_hash_map.
  explicit operator size_t() const { return id; }

  bool operator==(const Id<T>& other) const { return id == other.id; }

  bool operator<(const Id<T>& other) const { return id < other.id; }
};

namespace std {
template <typename T>
struct hash<Id<T>> {
  size_t operator()(const Id<T>& k) const { return hash<size_t>()(k.id); }
};
}  // namespace std

template <typename T>
bool operator==(const Id<T>& a, const Id<T>& b) {
  assert(a.group == b.group && "Cannot compare Ids from different groups");
  return a.id == b.id;
}
template <typename T>
bool operator!=(const Id<T>& a, const Id<T>& b) {
  return !(a == b);
}

template <typename T>
void Reflect(Reader& visitor, Id<T>& id) {
  if (visitor.IsUint64())
    id.id = visitor.GetUint64();
}
template <typename T>
void Reflect(Writer& visitor, Id<T>& value) {
  visitor.Uint64(value.id);
}

using IndexTypeId = Id<IndexType>;
using IndexFuncId = Id<IndexFunc>;
using IndexVarId = Id<IndexVar>;

struct IdCache;

struct IndexFuncRef {
  // NOTE: id can be -1 if the function call is not coming from a function.
  IndexFuncId id;
  Range loc;
  bool is_implicit = false;

  IndexFuncRef() {}  // For serialization.

  IndexFuncRef(IndexFuncId id, Range loc, bool is_implicit)
      : id(id), loc(loc), is_implicit(is_implicit) {}
  IndexFuncRef(Range loc, bool is_implicit)
      : id(IndexFuncId((size_t)-1)), loc(loc), is_implicit(is_implicit) {}

  inline bool operator==(const IndexFuncRef& other) {
    return id == other.id && loc == other.loc &&
           is_implicit == other.is_implicit;
  }
  inline bool operator!=(const IndexFuncRef& other) {
    return !(*this == other);
  }
  inline bool operator<(const IndexFuncRef& other) const {
    if (id < other.id)
      return true;
    if (id == other.id && loc < other.loc)
      return true;
    return id == other.id && loc == other.loc &&
           is_implicit < other.is_implicit;
  }
};

inline bool operator==(const IndexFuncRef& a, const IndexFuncRef& b) {
  return a.id == b.id && a.loc == b.loc;
}
inline bool operator!=(const IndexFuncRef& a, const IndexFuncRef& b) {
  return !(a == b);
}

inline void Reflect(Reader& visitor, IndexFuncRef& value) {
  const char* str_value = visitor.GetString();
  if (str_value[0] == '~') {
    value.is_implicit = true;
    ++str_value;
  }
  uint64_t id = atol(str_value);
  const char* loc_string = strchr(str_value, '@') + 1;

  value.id = IndexFuncId(id);
  value.loc = Range(loc_string);
}
inline void Reflect(Writer& visitor, IndexFuncRef& value) {
  std::string s;

  if (value.is_implicit)
    s += "~";

  // id.id is unsigned, special case 0 value
  if (static_cast<ssize_t>(value.id.id) == -1) {
    s += "-1";
  } else {
    s += std::to_string(value.id.id);
  }

  s += "@" + value.loc.ToString();
  visitor.String(s.c_str());
}

template <typename TypeId, typename FuncId, typename VarId, typename Range>
struct TypeDefDefinitionData {
  // General metadata.
  std::string usr;
  std::string short_name;
  std::string detailed_name;

  // While a class/type can technically have a separate declaration/definition,
  // it doesn't really happen in practice. The declaration never contains
  // comments or insightful information. The user always wants to jump from
  // the declaration to the definition - never the other way around like in
  // functions and (less often) variables.
  //
  // It's also difficult to identify a `class Foo;` statement with the clang
  // indexer API (it's doable using cursor AST traversal), so we don't bother
  // supporting the feature.
  optional<Range> definition_spelling;
  optional<Range> definition_extent;

  // If set, then this is the same underlying type as the given value (ie, this
  // type comes from a using or typedef statement).
  optional<TypeId> alias_of;

  // Immediate parent types.
  std::vector<TypeId> parents;

  // Types, functions, and variables defined in this type.
  std::vector<TypeId> types;
  std::vector<FuncId> funcs;
  std::vector<VarId> vars;

  TypeDefDefinitionData() {}  // For reflection.
  TypeDefDefinitionData(const std::string& usr) : usr(usr) {}

  bool operator==(
      const TypeDefDefinitionData<TypeId, FuncId, VarId, Range>& other) const {
    return usr == other.usr && short_name == other.short_name &&
           detailed_name == other.detailed_name &&
           definition_spelling == other.definition_spelling &&
           definition_extent == other.definition_extent &&
           alias_of == other.alias_of && parents == other.parents &&
           types == other.types && funcs == other.funcs && vars == other.vars;
  }

  bool operator!=(
      const TypeDefDefinitionData<TypeId, FuncId, VarId, Range>& other) const {
    return !(*this == other);
  }
};
template <typename TVisitor,
          typename TypeId,
          typename FuncId,
          typename VarId,
          typename Range>
void Reflect(TVisitor& visitor,
             TypeDefDefinitionData<TypeId, FuncId, VarId, Range>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(usr);
  REFLECT_MEMBER(short_name);
  REFLECT_MEMBER(detailed_name);
  REFLECT_MEMBER(definition_spelling);
  REFLECT_MEMBER(definition_extent);
  REFLECT_MEMBER(alias_of);
  REFLECT_MEMBER(parents);
  REFLECT_MEMBER(types);
  REFLECT_MEMBER(funcs);
  REFLECT_MEMBER(vars);
  REFLECT_MEMBER_END();
}

struct IndexType {
  using Def =
      TypeDefDefinitionData<IndexTypeId, IndexFuncId, IndexVarId, Range>;
  Def def;

  IndexTypeId id;

  // Immediate derived types.
  std::vector<IndexTypeId> derived;

  // Declared variables of this type.
  std::vector<IndexVarId> instances;

  // Every usage, useful for things like renames.
  // NOTE: Do not insert directly! Use AddUsage instead.
  std::vector<Range> uses;

  IndexType() : def("") {}  // For serialization

  IndexType(IndexTypeId id, const std::string& usr);

  bool operator<(const IndexType& other) const {
    return def.usr < other.def.usr;
  }
};

MAKE_HASHABLE(IndexType, t.def.usr);

template <typename TypeId,
          typename FuncId,
          typename VarId,
          typename FuncRef,
          typename Range>
struct FuncDefDefinitionData {
  // General metadata.
  std::string usr;
  std::string short_name;
  std::string detailed_name;
  optional<Range> definition_spelling;
  optional<Range> definition_extent;

  // Type which declares this one (ie, it is a method)
  optional<TypeId> declaring_type;

  // Method this method overrides.
  optional<FuncId> base;

  // Local variables defined in this function.
  std::vector<VarId> locals;

  // Functions that this function calls.
  std::vector<FuncRef> callees;

  FuncDefDefinitionData() {}  // For reflection.
  FuncDefDefinitionData(const std::string& usr) : usr(usr) {
    // assert(usr.size() > 0);
  }

  bool operator==(
      const FuncDefDefinitionData<TypeId, FuncId, VarId, FuncRef, Range>& other)
      const {
    return usr == other.usr && short_name == other.short_name &&
           detailed_name == other.detailed_name &&
           definition_spelling == other.definition_spelling &&
           definition_extent == other.definition_extent &&
           declaring_type == other.declaring_type && base == other.base &&
           locals == other.locals && callees == other.callees;
  }
  bool operator!=(
      const FuncDefDefinitionData<TypeId, FuncId, VarId, FuncRef, Range>& other)
      const {
    return !(*this == other);
  }
};

template <typename TVisitor,
          typename TypeId,
          typename FuncId,
          typename VarId,
          typename FuncRef,
          typename Range>
void Reflect(
    TVisitor& visitor,
    FuncDefDefinitionData<TypeId, FuncId, VarId, FuncRef, Range>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(usr);
  REFLECT_MEMBER(short_name);
  REFLECT_MEMBER(detailed_name);
  REFLECT_MEMBER(definition_spelling);
  REFLECT_MEMBER(definition_extent);
  REFLECT_MEMBER(declaring_type);
  REFLECT_MEMBER(base);
  REFLECT_MEMBER(locals);
  REFLECT_MEMBER(callees);
  REFLECT_MEMBER_END();
}

struct IndexFunc {
  using Def = FuncDefDefinitionData<IndexTypeId,
                                    IndexFuncId,
                                    IndexVarId,
                                    IndexFuncRef,
                                    Range>;
  Def def;

  IndexFuncId id;

  struct Declaration {
    // Range of only the function name.
    Range spelling;
    // Full range of the declaration.
    Range extent;
    // Full text of the declaration.
    std::string content;
    // Location of the parameter names.
    std::vector<Range> param_spellings;
  };

  // Places the function is forward-declared.
  std::vector<Declaration> declarations;

  // Methods which directly override this one.
  std::vector<IndexFuncId> derived;

  // Calls/usages of this function. If the call is coming from outside a
  // function context then the FuncRef will not have an associated id.
  //
  // To get all usages, also include the ranges inside of declarations and
  // def.definition_spelling.
  std::vector<IndexFuncRef> callers;

  IndexFunc() {}  // For reflection.
  IndexFunc(IndexFuncId id, const std::string& usr) : def(usr), id(id) {
    // assert(usr.size() > 0);
  }

  bool operator<(const IndexFunc& other) const {
    return def.usr < other.def.usr;
  }
};
MAKE_HASHABLE(IndexFunc, t.def.usr);
MAKE_REFLECT_STRUCT(IndexFunc::Declaration,
                    spelling,
                    extent,
                    content,
                    param_spellings);

template <typename TypeId, typename FuncId, typename VarId, typename Range>
struct VarDefDefinitionData {
  // General metadata.
  std::string usr;
  std::string short_name;
  std::string detailed_name;
  optional<Range> declaration;
  // TODO: definitions should be a list of ranges, since there can be more
  //       than one - when??
  optional<Range> definition_spelling;
  optional<Range> definition_extent;

  // Type of the variable.
  optional<TypeId> variable_type;

  // Type which declares this one.
  optional<TypeId> declaring_type;

  // Is this a parameter or function variable?
  bool is_local = false;
  // Is this a macro, ie, #define FOO?
  bool is_macro = false;

  VarDefDefinitionData() {}  // For reflection.
  VarDefDefinitionData(const std::string& usr) : usr(usr) {}

  bool operator==(
      const VarDefDefinitionData<TypeId, FuncId, VarId, Range>& other) const {
    return usr == other.usr && short_name == other.short_name &&
           detailed_name == other.detailed_name &&
           declaration == other.declaration &&
           definition_spelling == other.definition_spelling &&
           definition_extent == other.definition_extent &&
           variable_type == other.variable_type &&
           declaring_type == other.declaring_type;
  }
  bool operator!=(
      const VarDefDefinitionData<TypeId, FuncId, VarId, Range>& other) const {
    return !(*this == other);
  }
};

template <typename TVisitor,
          typename TypeId,
          typename FuncId,
          typename VarId,
          typename Range>
void Reflect(TVisitor& visitor,
             VarDefDefinitionData<TypeId, FuncId, VarId, Range>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(usr);
  REFLECT_MEMBER(short_name);
  REFLECT_MEMBER(detailed_name);
  REFLECT_MEMBER(definition_spelling);
  REFLECT_MEMBER(definition_extent);
  REFLECT_MEMBER(variable_type);
  REFLECT_MEMBER(declaring_type);
  REFLECT_MEMBER(is_local);
  REFLECT_MEMBER(is_macro);
  REFLECT_MEMBER_END();
}

struct IndexVar {
  using Def = VarDefDefinitionData<IndexTypeId, IndexFuncId, IndexVarId, Range>;
  Def def;

  IndexVarId id;

  // Usages.
  std::vector<Range> uses;

  IndexVar() : def("") {}  // For serialization

  IndexVar(IndexVarId id, const std::string& usr) : def(usr), id(id) {
    // assert(usr.size() > 0);
  }

  bool operator<(const IndexVar& other) const {
    return def.usr < other.def.usr;
  }
};
MAKE_HASHABLE(IndexVar, t.def.usr);

struct IdCache {
  std::string primary_file;
  std::unordered_map<std::string, IndexTypeId> usr_to_type_id;
  std::unordered_map<std::string, IndexFuncId> usr_to_func_id;
  std::unordered_map<std::string, IndexVarId> usr_to_var_id;
  std::unordered_map<IndexTypeId, std::string> type_id_to_usr;
  std::unordered_map<IndexFuncId, std::string> func_id_to_usr;
  std::unordered_map<IndexVarId, std::string> var_id_to_usr;

  IdCache(const std::string& primary_file);
};

struct IndexInclude {
  // Line that has the include directive. We don't have complete range
  // information - a line is good enough for clicking.
  int line = 0;
  // Absolute path to the index.
  std::string resolved_path;
};
MAKE_REFLECT_STRUCT(IndexInclude, line, resolved_path);

struct IndexFile {
  IdCache id_cache;

  static int kCurrentVersion;
  int version = 0;

  std::string path;
  std::vector<std::string> args;
  int64_t last_modification_time = 0;

  // The path to the translation unit cc file which caused the creation of this
  // IndexFile. When parsing a translation unit we generate many IndexFile
  // instances (ie, each header has a separate one). When the user edits a
  // header we need to lookup the original translation unit and reindex that.
  std::string import_file;

  // Source ranges that were not processed.
  std::vector<Range> skipped_by_preprocessor;

  std::vector<IndexInclude> includes;
  std::vector<std::string> dependencies;
  std::vector<IndexType> types;
  std::vector<IndexFunc> funcs;
  std::vector<IndexVar> vars;

  // Diagnostics found when indexing this file. Not serialized.
  NonElidedVector<lsDiagnostic> diagnostics_;
  // File contents at the time of index. Not serialized.
  std::string file_contents_;

  IndexFile(const std::string& path);

  IndexTypeId ToTypeId(const std::string& usr);
  IndexFuncId ToFuncId(const std::string& usr);
  IndexVarId ToVarId(const std::string& usr);
  IndexTypeId ToTypeId(const CXCursor& usr);
  IndexFuncId ToFuncId(const CXCursor& usr);
  IndexVarId ToVarId(const CXCursor& usr);
  IndexType* Resolve(IndexTypeId id);
  IndexFunc* Resolve(IndexFuncId id);
  IndexVar* Resolve(IndexVarId id);

  std::string ToString();
};

struct FileContents {
  std::string path;
  std::string content;

  FileContents(const std::string& path, const std::string& content);
};

// |import_file| is the cc file which is what gets passed to clang.
// |desired_index_file| is the (h or cc) file which has actually changed.
// |dependencies| are the existing dependencies of |import_file| if this is a
// reparse.
std::vector<std::unique_ptr<IndexFile>> Parse(
    Config* config,
    FileConsumer::SharedState* file_consumer_shared,
    std::string file,
    const std::vector<std::string>& args,
    const std::vector<FileContents>& file_contents,
    PerformanceImportFile* perf,
    ClangIndex* index,
    bool dump_ast = false);
std::vector<std::unique_ptr<IndexFile>> ParseWithTu(
    FileConsumer::SharedState* file_consumer_shared,
    PerformanceImportFile* perf,
    ClangTranslationUnit* tu,
    ClangIndex* index,
    const std::string& file,
    const std::vector<std::string>& args,
    const std::vector<CXUnsavedFile>& file_contents);

void IndexInit();
