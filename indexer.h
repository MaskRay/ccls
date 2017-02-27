#pragma once

#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <fstream>
#include <unordered_map>

#include "libclangmm/clangmm.h"
#include "libclangmm/Utility.h"

#include "bitfield.h"
#include "utils.h"
#include "optional.h"

#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>

struct IndexedTypeDef;
struct IndexedFuncDef;
struct IndexedVarDef;

using namespace std::experimental;


using GroupId = int;

template<typename T>
struct Id {
  GroupId group;
  uint64_t id;

  Id() : id(0) {} // Needed for containers. Do not use directly.
  Id(GroupId group, uint64_t id) : group(group), id(id) {}

  bool operator==(const Id<T>& other) const {
    assert(group == other.group && "Cannot compare Ids from different groups");
    return id == other.id;
  }

  bool operator<(const Id<T>& other) const {
    assert(group == other.group);
    return id < other.id;
  }
};

namespace std {
  template<typename T>
  struct hash<Id<T>> {
    size_t operator()(const Id<T>& k) const {
      return ((hash<uint64_t>()(k.id) ^ (hash<int>()(k.group) << 1)) >> 1);
    }
  };
}


template<typename T>
bool operator==(const Id<T>& a, const Id<T>& b) {
  assert(a.group == b.group && "Cannot compare Ids from different groups");
  return a.id == b.id;
}

struct _FakeFileType {};
using FileId = Id<_FakeFileType>;
using TypeId = Id<IndexedTypeDef>;
using FuncId = Id<IndexedFuncDef>;
using VarId = Id<IndexedVarDef>;

struct Location {
  bool interesting;
  int raw_file_group;
  int raw_file_id;
  int line;
  int column;

  Location() {
    interesting = false;
    raw_file_group = -1;
    raw_file_id = -1;
    line = -1;
    column = -1;
  }

  Location(bool interesting, FileId file, uint32_t line, uint32_t column) {
    this->interesting = interesting;
    this->raw_file_group = file.group;
    this->raw_file_id = file.id;
    this->line = line;
    this->column = column;
  }

  FileId file_id() {
    return FileId(raw_file_id, raw_file_group);
  }

  std::string ToString() {
    // Output looks like this:
    //
    //  *1:2:3
    //
    // * => interesting
    // 1 => file id
    // 2 => line
    // 3 => column

    std::string result;
    if (interesting)
      result += '*';
    result += std::to_string(raw_file_id);
    result += ':';
    result += std::to_string(line);
    result += ':';
    result += std::to_string(column);
    return result;
  }

  // Compare two Locations and check if they are equal. Ignores the value of
  // |interesting|.
  // operator== doesn't seem to work properly...
  bool IsEqualTo(const Location& o) const {
    // When comparing, ignore the value of |interesting|.
    return
      raw_file_group == o.raw_file_group &&
      raw_file_id == o.raw_file_id &&
      line == o.line &&
      column == o.column;
  }

  bool operator==(const Location& o) const {
    return IsEqualTo(o);
  }
  bool operator<(const Location& o) const {
    return
      interesting < o.interesting &&
      raw_file_group < o.raw_file_group &&
      raw_file_id < o.raw_file_id &&
      line < o.line &&
      column < o.column;
  }

  Location WithInteresting(bool interesting) {
    Location result = *this;
    result.interesting = interesting;
    return result;
  }
};

#if false
// TODO: Move off of this weird wrapper, use struct with custom wrappers
//       directly.
BEGIN_BITFIELD_TYPE(Location, uint64_t)

ADD_BITFIELD_MEMBER(interesting,    /*start:*/ 0,  /*len:*/ 1);    // 2 values
ADD_BITFIELD_MEMBER(raw_file_group, /*start:*/ 1,  /*len:*/ 4);    // 16 values, ok if they wrap around.
ADD_BITFIELD_MEMBER(raw_file_id,    /*start:*/ 5,  /*len:*/ 25);   // 33,554,432 values
ADD_BITFIELD_MEMBER(line,           /*start:*/ 30, /*len:*/ 20);   // 1,048,576 values
ADD_BITFIELD_MEMBER(column,         /*start:*/ 50, /*len:*/ 14);   // 16,384 values

Location(bool interesting, FileId file, uint32_t line, uint32_t column) {
  this->interesting = interesting;
  this->raw_file_group = file.group;
  this->raw_file_id = file.id;
  this->line = line;
  this->column = column;
}

FileId file_id() {
  return FileId(raw_file_id, raw_file_group);
}

std::string ToString() {
  // Output looks like this:
  //
  //  *1:2:3
  //
  // * => interesting
  // 1 => file id
  // 2 => line
  // 3 => column

  std::string result;
  if (interesting)
    result += '*';
  result += std::to_string(raw_file_id);
  result += ':';
  result += std::to_string(line);
  result += ':';
  result += std::to_string(column);
  return result;
}

// Compare two Locations and check if they are equal. Ignores the value of
// |interesting|.
// operator== doesn't seem to work properly...
bool IsEqualTo(const Location& o) {
  // When comparing, ignore the value of |interesting|.
  return (wrapper.value >> 1) == (o.wrapper.value >> 1);
}

Location WithInteresting(bool interesting) {
  Location result = *this;
  result.interesting = interesting;
  return result;
}

END_BITFIELD_TYPE()
#endif

template<typename T>
struct Ref {
  Id<T> id;
  Location loc;

  Ref(Id<T> id, Location loc) : id(id), loc(loc) {}

  bool operator==(const Ref<T>& other) {
    return id == other.id && loc == other.loc;
  }
  bool operator!=(const Ref<T>& other) {
    return !(*this == other);
  }
  bool operator<(const Ref<T>& other) const {
    return id < other.id && loc < other.loc;
  }
};

template<typename T>
bool operator==(const Ref<T>& a, const Ref<T>& b) {
  return a.id == b.id && a.loc == b.loc;
}
template<typename T>
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

template<typename TypeId = TypeId, typename FuncId = FuncId, typename VarId = VarId, typename Location = Location>
struct TypeDefDefinitionData {
  // General metadata.
  TypeId id;
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
  optional<Location> definition;

  // If set, then this is the same underlying type as the given value (ie, this
  // type comes from a using or typedef statement).
  optional<TypeId> alias_of;

  // Immediate parent types.
  std::vector<TypeId> parents;

  // Types, functions, and variables defined in this type.
  std::vector<TypeId> types;
  std::vector<FuncId> funcs;
  std::vector<VarId> vars;

  TypeDefDefinitionData(TypeId id, const std::string& usr) : id(id), usr(usr) {}

  bool operator==(const TypeDefDefinitionData<TypeId, FuncId, VarId>& other) const {
    return
      id == other.id &&
      usr == other.usr &&
      short_name == other.short_name &&
      qualified_name == other.qualified_name &&
      definition == other.definition &&
      alias_of == other.alias_of &&
      parents == other.parents &&
      types == other.types &&
      funcs == other.funcs &&
      vars == other.vars;
  }

  bool operator!=(const TypeDefDefinitionData<TypeId, FuncId, VarId>& other) const {
    return !(*this == other);
  }
};

struct IndexedTypeDef {
  TypeDefDefinitionData<> def;

  // Immediate derived types.
  std::vector<TypeId> derived;

  // Every usage, useful for things like renames.
  // NOTE: Do not insert directly! Use AddUsage instead.
  std::vector<Location> uses;

  bool is_bad_def = true;

  IndexedTypeDef(TypeId id, const std::string& usr);
  void AddUsage(Location loc, bool insert_if_not_present = true);

  bool operator<(const IndexedTypeDef& other) const {
    return def.id < other.def.id;
  }
};

namespace std {
  template <>
  struct hash<IndexedTypeDef> {
    size_t operator()(const IndexedTypeDef& k) const {
      return hash<string>()(k.def.usr);
    }
  };
}

template<typename TypeId = TypeId, typename FuncId = FuncId, typename VarId = VarId, typename FuncRef = FuncRef, typename Location = Location>
struct FuncDefDefinitionData {
  // General metadata.
  FuncId id;
  std::string usr;
  std::string short_name;
  std::string qualified_name;
  optional<Location> definition;

  // Type which declares this one (ie, it is a method)
  optional<TypeId> declaring_type;

  // Method this method overrides.
  optional<FuncId> base;

  // Local variables defined in this function.
  std::vector<VarId> locals;

  // Functions that this function calls.
  std::vector<FuncRef> callees;

  FuncDefDefinitionData(FuncId id, const std::string& usr) : id(id), usr(usr) {
    assert(usr.size() > 0);
  }

  bool operator==(const FuncDefDefinitionData<TypeId, FuncId, VarId, FuncRef>& other) const {
    return
      id == other.id &&
      usr == other.usr &&
      short_name == other.short_name &&
      qualified_name == other.qualified_name &&
      definition == other.definition &&
      declaring_type == other.declaring_type &&
      base == other.base &&
      locals == other.locals &&
      callees == other.callees;
  }

  bool operator!=(const FuncDefDefinitionData<TypeId, FuncId, VarId, FuncRef>& other) const {
    return !(*this == other);
  }
};

struct IndexedFuncDef {
  FuncDefDefinitionData<> def;

  // Places the function is forward-declared.
  std::vector<Location> declarations;

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
  std::vector<Location> uses;

  bool is_bad_def = true;

  IndexedFuncDef(FuncId id, const std::string& usr) : def(id, usr) {
    assert(usr.size() > 0);
  }

  bool operator<(const IndexedFuncDef& other) const {
    return def.id < other.def.id;
  }
};

namespace std {
  template <>
  struct hash<IndexedFuncDef> {
    size_t operator()(const IndexedFuncDef& k) const {
      return hash<string>()(k.def.usr);
    }
  };
}

template<typename TypeId = TypeId, typename FuncId = FuncId, typename VarId = VarId, typename Location = Location>
struct VarDefDefinitionData {
  // General metadata.
  VarId id;
  std::string usr;
  std::string short_name;
  std::string qualified_name;
  optional<Location> declaration;
  // TODO: definitions should be a list of locations, since there can be more
  //       than one.
  optional<Location> definition;

  // Type of the variable.
  optional<TypeId> variable_type;

  // Type which declares this one (ie, it is a method)
  optional<TypeId> declaring_type;

  VarDefDefinitionData(VarId id, const std::string& usr) : id(id), usr(usr) {}

  bool operator==(const VarDefDefinitionData<TypeId, FuncId, VarId>& other) const {
    return
      id == other.id &&
      usr == other.usr &&
      short_name == other.short_name &&
      qualified_name == other.qualified_name &&
      declaration == other.declaration &&
      definition == other.definition &&
      variable_type == other.variable_type &&
      declaring_type == other.declaring_type;
  }

  bool operator!=(const VarDefDefinitionData<TypeId, FuncId, VarId>& other) const {
    return !(*this == other);
  }
};

struct IndexedVarDef {
  VarDefDefinitionData<> def;

  // Usages.
  std::vector<Location> uses;

  bool is_bad_def = true;

  IndexedVarDef(VarId id, const std::string& usr) : def(id, usr) {
    assert(usr.size() > 0);
  }

  bool operator<(const IndexedVarDef& other) const {
    return def.id < other.def.id;
  }
};

namespace std {
  template <>
  struct hash<IndexedVarDef> {
    size_t operator()(const IndexedVarDef& k) const {
      return hash<string>()(k.def.usr);
    }
  };
}

struct IdCache {
  // NOTE: Every Id is resolved to a file_id of 0. The correct file_id needs
  //       to get fixed up when inserting into the real db.
  GroupId group;
  std::unordered_map<std::string, FileId> file_path_to_file_id;
  std::unordered_map<std::string, TypeId> usr_to_type_id;
  std::unordered_map<std::string, FuncId> usr_to_func_id;
  std::unordered_map<std::string, VarId> usr_to_var_id;
  std::unordered_map<FileId, std::string> file_id_to_file_path;
  std::unordered_map<TypeId, std::string> type_id_to_usr;
  std::unordered_map<FuncId, std::string> func_id_to_usr;
  std::unordered_map<VarId, std::string> var_id_to_usr;

  IdCache(GroupId group) : group(group) {
    // Reserve id 0 for unfound.
    file_path_to_file_id[""] = FileId(group, 0);
    file_id_to_file_path[FileId(group, 0)] = "";
  }

  Location Resolve(const CXSourceLocation& cx_loc, bool interesting) {
    CXFile file;
    unsigned int line, column, offset;
    clang_getSpellingLocation(cx_loc, &file, &line, &column, &offset);

    FileId file_id(-1, -1);
    if (file != nullptr) {
      std::string path = clang::ToString(clang_getFileName(file));

      auto it = file_path_to_file_id.find(path);
      if (it != file_path_to_file_id.end()) {
        file_id = it->second;
      }
      else {
        file_id = FileId(group, file_path_to_file_id.size());
        file_path_to_file_id[path] = file_id;
        file_id_to_file_path[file_id] = path;
      }
    }

    return Location(interesting, file_id, line, column);
  }

  Location Resolve(const CXIdxLoc& cx_idx_loc, bool interesting) {
    CXSourceLocation cx_loc = clang_indexLoc_getCXSourceLocation(cx_idx_loc);
    return Resolve(cx_loc, interesting);
  }

  Location Resolve(const CXCursor& cx_cursor, bool interesting) {
    return Resolve(clang_getCursorLocation(cx_cursor), interesting);
  }

  Location Resolve(const clang::Cursor& cursor, bool interesting) {
    return Resolve(cursor.cx_cursor, interesting);
  }

};

struct IndexedFile {
  IdCache* id_cache;

  std::vector<IndexedTypeDef> types;
  std::vector<IndexedFuncDef> funcs;
  std::vector<IndexedVarDef> vars;

  IndexedFile(IdCache* id_cache);

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



IndexedFile Parse(IdCache* id_cache, std::string filename, std::vector<std::string> args);