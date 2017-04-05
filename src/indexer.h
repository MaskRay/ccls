#pragma once

#include "position.h"
#include "serializer.h"
#include "utils.h"
#include "libclangmm/clangmm.h"
#include "libclangmm/Utility.h"

#include <optional.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>

#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <fstream>
#include <unordered_map>

struct IndexedTypeDef;
struct IndexedFuncDef;
struct IndexedVarDef;

using namespace std::experimental;

template <typename T>
struct Id {
  uint64_t id;

  Id() : id(0) {}  // Needed for containers. Do not use directly.
  Id(uint64_t id) : id(id) {}

  bool operator==(const Id<T>& other) const { return id == other.id; }

  bool operator<(const Id<T>& other) const { return id < other.id; }
};

namespace std {
template <typename T>
struct hash<Id<T>> {
  size_t operator()(const Id<T>& k) const { return hash<uint64_t>()(k.id); }
};
}

template <typename T>
bool operator==(const Id<T>& a, const Id<T>& b) {
  assert(a.group == b.group && "Cannot compare Ids from different groups");
  return a.id == b.id;
}

struct _FakeFileType {};
using TypeId = Id<IndexedTypeDef>;
using FuncId = Id<IndexedFuncDef>;
using VarId = Id<IndexedVarDef>;

struct IdCache;

template <typename T>
struct Ref {
  Id<T> id;
  Range loc;

  Ref() {}  // For serialization.

  Ref(Id<T> id, Range loc) : id(id), loc(loc) {}

  bool operator==(const Ref<T>& other) {
    return id == other.id && loc == other.loc;
  }
  bool operator!=(const Ref<T>& other) { return !(*this == other); }
  bool operator<(const Ref<T>& other) const {
    return id < other.id && loc < other.loc;
  }
};

template <typename T>
bool operator==(const Ref<T>& a, const Ref<T>& b) {
  return a.id == b.id && a.loc == b.loc;
}
template <typename T>
bool operator!=(const Ref<T>& a, const Ref<T>& b) {
  return !(a == b);
}

using TypeRef = Ref<IndexedTypeDef>;
using FuncRef = Ref<IndexedFuncDef>;
using VarRef = Ref<IndexedVarDef>;

// TODO: skip as much forward-processing as possible when |is_system_def| is
//       set to false.
// TODO: Either eliminate the defs created as a by-product of cross-referencing,
//       or do not emit things we don't have definitions for.

template <typename TypeId,
          typename FuncId,
          typename VarId,
          typename Position,
          typename Range>
struct TypeDefDefinitionData {
  // General metadata.
  std::string usr;
  std::string short_name;
  std::string qualified_name;

  // While a class/type can technically have a separate declaration/definition,
  // it doesn't really happen in practice. The declaration never contains
  // comments or insightful information. The user always wants to jump from
  // the declaration to the definition - never the other way around like in
  // functions and (less often) variables.
  //
  // It's also difficult to identify a `class Foo;` statement with the clang
  // indexer API (it's doable using cursor AST traversal), so we don't bother
  // supporting the feature.
  optional<Position> definition_spelling;
  optional<Range> definition_extent;

  // TODO: change |definition| to be a Position, and have an |extents| field which
  // stores the range of the definition body. Also do this for methods.
  // TODO: cleanup Range, Position, etc to take less memory. Model vscode api of
  // Location, Range, Position.
  // TODO: drop paths from everything in the index. We never store things outside
  // of the main file.
  // TODO: fix tests, the change to ranges is breaking them. Breaking currently
  // coming from marking

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

  bool operator==(const TypeDefDefinitionData<TypeId, FuncId, VarId, Position, Range>&
                      other) const {
    return usr == other.usr && short_name == other.short_name &&
           qualified_name == other.qualified_name &&
           definition_spelling == other.definition_spelling &&
           definition_extent == other.definition_extent &&
           alias_of == other.alias_of &&
           parents == other.parents && types == other.types &&
           funcs == other.funcs && vars == other.vars;
  }

  bool operator!=(const TypeDefDefinitionData<TypeId, FuncId, VarId, Position, Range>&
                      other) const {
    return !(*this == other);
  }
};
template <typename TVisitor,
          typename TypeId,
          typename FuncId,
          typename VarId,
          typename Position,
          typename Range>
void Reflect(TVisitor& visitor,
             TypeDefDefinitionData<TypeId, FuncId, VarId, Position, Range>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(usr);
  REFLECT_MEMBER(short_name);
  REFLECT_MEMBER(qualified_name);
  REFLECT_MEMBER(definition);
  REFLECT_MEMBER(alias_of);
  REFLECT_MEMBER(parents);
  REFLECT_MEMBER(types);
  REFLECT_MEMBER(funcs);
  REFLECT_MEMBER(vars);
  REFLECT_MEMBER_END();
}

struct IndexedTypeDef {
  using Def = TypeDefDefinitionData<TypeId, FuncId, VarId, Range, Range>;
  Def def;

  TypeId id;

  // Immediate derived types.
  std::vector<TypeId> derived;

  // Declared variables of this type.
  // TODO: this needs a lot more work and lots of tests.
  // TODO: add instantiation on ctor / dtor, do not add instantiation if type is ptr
  std::vector<VarId> instantiations;

  // Every usage, useful for things like renames.
  // NOTE: Do not insert directly! Use AddUsage instead.
  std::vector<Range> uses;

  IndexedTypeDef() : def("") {}  // For serialization

  IndexedTypeDef(TypeId id, const std::string& usr);

  bool HasInterestingState() const {
    return
      def.definition_spelling ||
      def.definition_extent ||
      !derived.empty() ||
      !instantiations.empty() ||
      !uses.empty();
  }

  bool operator<(const IndexedTypeDef& other) const {
    return def.usr < other.def.usr;
  }
};

MAKE_HASHABLE(IndexedTypeDef, t.def.usr);

template <typename TypeId,
          typename FuncId,
          typename VarId,
          typename FuncRef,
          typename Position,
          typename Range>
struct FuncDefDefinitionData {
  // General metadata.
  std::string usr;
  std::string short_name;
  std::string qualified_name;
  optional<Position> definition_spelling;
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
      const FuncDefDefinitionData<TypeId, FuncId, VarId, FuncRef, Position, Range>&
          other) const {
    return usr == other.usr && short_name == other.short_name &&
           qualified_name == other.qualified_name &&
           definition_spelling == other.definition_spelling &&
           definition_extent == other.definition_extent &&
           declaring_type == other.declaring_type && base == other.base &&
           locals == other.locals && callees == other.callees;
  }
  bool operator!=(
      const FuncDefDefinitionData<TypeId, FuncId, VarId, FuncRef, Position, Range>&
          other) const {
    return !(*this == other);
  }
};

template <typename TVisitor,
          typename TypeId,
          typename FuncId,
          typename VarId,
          typename FuncRef,
          typename Position,
          typename Range>
void Reflect(
    TVisitor& visitor,
    FuncDefDefinitionData<TypeId, FuncId, VarId, FuncRef, Position, Range>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(usr);
  REFLECT_MEMBER(short_name);
  REFLECT_MEMBER(qualified_name);
  REFLECT_MEMBER(definition);
  REFLECT_MEMBER(declaring_type);
  REFLECT_MEMBER(base);
  REFLECT_MEMBER(locals);
  REFLECT_MEMBER(callees);
  REFLECT_MEMBER_END();
}

struct IndexedFuncDef {
  using Def = FuncDefDefinitionData<TypeId, FuncId, VarId, FuncRef, Range, Range>;
  Def def;

  FuncId id;

  // Places the function is forward-declared.
  std::vector<Range> declarations;

  // Methods which directly override this one.
  std::vector<FuncId> derived;

  // Functions which call this one.
  // TODO: Functions can get called outside of just functions - for example,
  //       they can get called in static context (maybe redirect to main?)
  //       or in class initializer list (redirect to class ctor?)
  //    - Right now those usages will not get listed here (but they should be
  //      inside of all_uses).
  std::vector<FuncRef> callers;

  // All usages. For interesting usages, see callees.
  std::vector<Range> uses;

  IndexedFuncDef() {}  // For reflection.
  IndexedFuncDef(FuncId id, const std::string& usr) : def(usr), id(id) {
    // assert(usr.size() > 0);
  }

  bool HasInterestingState() const {
    return
      def.definition_spelling ||
      def.definition_extent ||
      !def.callees.empty() ||
      !declarations.empty() ||
      !derived.empty() ||
      !callers.empty() ||
      !uses.empty();
  }

  bool operator<(const IndexedFuncDef& other) const {
    return def.usr < other.def.usr;
  }
};
MAKE_HASHABLE(IndexedFuncDef, t.def.usr);

template <typename TypeId,
          typename FuncId,
          typename VarId,
          typename Position,
          typename Range>
struct VarDefDefinitionData {
  // General metadata.
  std::string usr;
  std::string short_name;
  std::string qualified_name;
  optional<Range> declaration;
  // TODO: definitions should be a list of ranges, since there can be more
  //       than one - when??
  optional<Position> definition_spelling;
  optional<Range> definition_extent;

  // Type of the variable.
  optional<TypeId> variable_type;

  // Type which declares this one (ie, it is a method)
  optional<TypeId> declaring_type;

  VarDefDefinitionData() {}  // For reflection.
  VarDefDefinitionData(const std::string& usr) : usr(usr) {}

  bool operator==(const VarDefDefinitionData<TypeId, FuncId, VarId, Position, Range>&
                      other) const {
    return usr == other.usr && short_name == other.short_name &&
           qualified_name == other.qualified_name &&
           declaration == other.declaration &&
           definition_spelling == other.definition_spelling &&
           definition_extent == other.definition_extent &&
           variable_type == other.variable_type &&
           declaring_type == other.declaring_type;
  }
  bool operator!=(const VarDefDefinitionData<TypeId, FuncId, VarId, Position, Range>&
                      other) const {
    return !(*this == other);
  }
};

template <typename TVisitor,
          typename TypeId,
          typename FuncId,
          typename VarId,
          typename Position,
          typename Range>
void Reflect(TVisitor& visitor,
             VarDefDefinitionData<TypeId, FuncId, VarId, Position, Range>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(usr);
  REFLECT_MEMBER(short_name);
  REFLECT_MEMBER(qualified_name);
  REFLECT_MEMBER(definition_spelling);
  REFLECT_MEMBER(definition_extent);
  REFLECT_MEMBER(variable_type);
  REFLECT_MEMBER(declaring_type);
  REFLECT_MEMBER_END();
}

struct IndexedVarDef {
  using Def = VarDefDefinitionData<TypeId, FuncId, VarId, Range, Range>;
  Def def;

  VarId id;

  // Usages.
  std::vector<Range> uses;

  IndexedVarDef() : def("") {}  // For serialization

  IndexedVarDef(VarId id, const std::string& usr) : def(usr), id(id) {
    // assert(usr.size() > 0);
  }

  bool HasInterestingState() const {
    return
      def.definition_spelling ||
      def.definition_extent ||
      !uses.empty();
  }

  bool operator<(const IndexedVarDef& other) const {
    return def.usr < other.def.usr;
  }
};
MAKE_HASHABLE(IndexedVarDef, t.def.usr);

struct IdCache {
  std::string primary_file;
  std::unordered_map<std::string, TypeId> usr_to_type_id;
  std::unordered_map<std::string, FuncId> usr_to_func_id;
  std::unordered_map<std::string, VarId> usr_to_var_id;
  std::unordered_map<TypeId, std::string> type_id_to_usr;
  std::unordered_map<FuncId, std::string> func_id_to_usr;
  std::unordered_map<VarId, std::string> var_id_to_usr;

  IdCache(const std::string& primary_file);

  Range ForceResolve(const CXSourceRange& range, bool interesting);

  Range ForceResolveSpelling(const CXCursor& cx_cursor, bool interesting);
  optional<Range> ResolveSpelling(const CXCursor& cx_cursor, bool interesting);
  optional<Range> ResolveSpelling(const clang::Cursor& cursor, bool interesting);

  Range ForceResolveExtent(const CXCursor& cx_cursor, bool interesting);
  optional<Range> ResolveExtent(const CXCursor& cx_cursor, bool interesting);
  optional<Range> ResolveExtent(const clang::Cursor& cursor, bool interesting);
};

struct IndexedFile {
  IdCache id_cache;

  std::string path;

  std::vector<IndexedTypeDef> types;
  std::vector<IndexedFuncDef> funcs;
  std::vector<IndexedVarDef> vars;

  IndexedFile(const std::string& path);

  TypeId ToTypeId(const std::string& usr);
  FuncId ToFuncId(const std::string& usr);
  VarId ToVarId(const std::string& usr);
  TypeId ToTypeId(const CXCursor& usr);
  FuncId ToFuncId(const CXCursor& usr);
  VarId ToVarId(const CXCursor& usr);
  IndexedTypeDef* Resolve(TypeId id);
  IndexedFuncDef* Resolve(FuncId id);
  IndexedVarDef* Resolve(VarId id);

  std::string ToString();
};

IndexedFile Parse(std::string filename,
                  std::vector<std::string> args,
                  bool dump_ast = false);
