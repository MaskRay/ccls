#include <algorithm>
#include <optional>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <fstream>
#include <unordered_map>

#include "libclangmm/clangmm.h"
#include "libclangmm/Utility.h"

#include "bitfield.h"
#include "utils.h"

#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>

struct TypeDef;
struct FuncDef;
struct VarDef;

using FileId = int64_t;


// TODO: Move off of this weird wrapper, use struct with custom wrappers
//       directly.
BEGIN_BITFIELD_TYPE(Location, uint64_t)

ADD_BITFIELD_MEMBER(interesting, /*start:*/ 0,  /*len:*/ 1);    // 2 values
ADD_BITFIELD_MEMBER(file_id,     /*start:*/ 1,  /*len:*/ 29);   // 536,870,912 values
ADD_BITFIELD_MEMBER(line,        /*start:*/ 30, /*len:*/ 20);   // 1,048,576 values
ADD_BITFIELD_MEMBER(column,      /*start:*/ 50, /*len:*/ 14);   // 16,384 values

Location(bool interesting, FileId file_id, uint32_t line, uint32_t column) {
  this->interesting = interesting;
  this->file_id = file_id;
  this->line = line;
  this->column = column;
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
  result += std::to_string(file_id);
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

struct FileDb {
  std::unordered_map<std::string, FileId> file_path_to_file_id;
  std::unordered_map<FileId, std::string> file_id_to_file_path;

  FileDb() {
    // Reserve id 0 for unfound.
    file_path_to_file_id[""] = 0;
    file_id_to_file_path[0] = "";
  }

  Location Resolve(const CXSourceLocation& cx_loc, bool interesting) {
    CXFile file;
    unsigned int line, column, offset;
    clang_getSpellingLocation(cx_loc, &file, &line, &column, &offset);

    FileId file_id;
    if (file != nullptr) {
      std::string path = clang::ToString(clang_getFileName(file));

      auto it = file_path_to_file_id.find(path);
      if (it != file_path_to_file_id.end()) {
        file_id = it->second;
      }
      else {
        file_id = file_path_to_file_id.size();
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


template<typename T>
struct LocalId {
  uint64_t local_id;

  LocalId() : local_id(0) {} // Needed for containers. Do not use directly.
  explicit LocalId(uint64_t local_id) : local_id(local_id) {}

  bool operator==(const LocalId<T>& other) {
    return local_id == other.local_id;
  }
};

template<typename T>
bool operator==(const LocalId<T>& a, const LocalId<T>& b) {
  return a.local_id == b.local_id;
}

using TypeId = LocalId<TypeDef>;
using FuncId = LocalId<FuncDef>;
using VarId = LocalId<VarDef>;


template<typename T>
struct Ref {
  LocalId<T> id;
  Location loc;

  Ref(LocalId<T> id, Location loc) : id(id), loc(loc) {}
};
using TypeRef = Ref<TypeDef>;
using FuncRef = Ref<FuncDef>;
using VarRef = Ref<VarDef>;


// NOTE: declaration is empty if there is no forward declaration!

struct TypeDef {
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
  std::optional<Location> definition;

  // If set, then this is the same underlying type as the given value (ie, this
  // type comes from a using or typedef statement).
  std::optional<TypeId> alias_of;

  // Immediate parent and immediate derived types.
  std::vector<TypeId> parents;
  std::vector<TypeId> derived;

  // Types, functions, and variables defined in this type.
  std::vector<TypeId> types;
  std::vector<FuncId> funcs;
  std::vector<VarId> vars;

  // Every usage, useful for things like renames.
  // NOTE: Do not insert directly! Use AddUsage instead.
  std::vector<Location> uses;

  TypeDef(TypeId id, const std::string& usr) : id(id), usr(usr) {
    assert(usr.size() > 0);
    //std::cout << "Creating type with usr " << usr << std::endl;
  }

  void AddUsage(Location loc, bool insert_if_not_present = true) {
    for (int i = uses.size() - 1; i >= 0; --i) {
      if (uses[i].IsEqualTo(loc)) {
        if (loc.interesting)
          uses[i].interesting = true;
        return;
      }
    }

    if (insert_if_not_present)
      uses.push_back(loc);
  }
};

struct FuncDef {
  // General metadata.
  FuncId id;
  std::string usr;
  std::string short_name;
  std::string qualified_name;
  std::optional<Location> declaration;
  std::optional<Location> definition;

  // Type which declares this one (ie, it is a method)
  std::optional<TypeId> declaring_type;
  // Method this method overrides.
  std::optional<FuncId> base;
  // Methods which directly override this one.
  std::vector<FuncId> derived;

  // Local variables defined in this function.
  std::vector<VarId> locals;

  // Functions which call this one.
  // TODO: Functions can get called outside of just functions - for example,
  //       they can get called in static context (maybe redirect to main?)
  //       or in class initializer list (redirect to class ctor?)
  //    - Right now those usages will not get listed here (but they should be
  //      inside of all_uses).
  std::vector<FuncRef> callers;
  // Functions that this function calls.
  std::vector<FuncRef> callees;

  // All usages. For interesting usages, see callees.
  std::vector<Location> uses;

  FuncDef(FuncId id, const std::string& usr) : id(id), usr(usr) {
    assert(usr.size() > 0);
  }
};

struct VarDef {
  // General metadata.
  VarId id;
  std::string usr;
  std::string short_name;
  std::string qualified_name;
  std::optional<Location> declaration;
  // TODO: definitions should be a list of locations, since there can be more
  //       than one.
  std::optional<Location> definition;

  // Type of the variable.
  std::optional<TypeId> variable_type;

  // Type which declares this one (ie, it is a method)
  std::optional<TypeId> declaring_type;

  // Usages.
  std::vector<Location> uses;

  VarDef(VarId id, const std::string& usr) : id(id), usr(usr) {
    assert(usr.size() > 0);
  }
};


struct ParsingDatabase {
  // NOTE: Every Id is resolved to a file_id of 0. The correct file_id needs
  //       to get fixed up when inserting into the real db.
  std::unordered_map<std::string, TypeId> usr_to_type_id;
  std::unordered_map<std::string, FuncId> usr_to_func_id;
  std::unordered_map<std::string, VarId> usr_to_var_id;

  std::vector<TypeDef> types;
  std::vector<FuncDef> funcs;
  std::vector<VarDef> vars;

  FileDb file_db;

  ParsingDatabase();

  TypeId ToTypeId(const std::string& usr);
  FuncId ToFuncId(const std::string& usr);
  VarId ToVarId(const std::string& usr);
  TypeId ToTypeId(const CXCursor& usr);
  FuncId ToFuncId(const CXCursor& usr);
  VarId ToVarId(const CXCursor& usr);

  TypeDef* Resolve(TypeId id);
  FuncDef* Resolve(FuncId id);
  VarDef* Resolve(VarId id);

  std::string ToString();
};

ParsingDatabase::ParsingDatabase() {}

// TODO: Optimize for const char*?
TypeId ParsingDatabase::ToTypeId(const std::string& usr) {
  auto it = usr_to_type_id.find(usr);
  if (it != usr_to_type_id.end())
    return it->second;

  TypeId id(types.size());
  types.push_back(TypeDef(id, usr));
  usr_to_type_id[usr] = id;
  return id;
}
FuncId ParsingDatabase::ToFuncId(const std::string& usr) {
  auto it = usr_to_func_id.find(usr);
  if (it != usr_to_func_id.end())
    return it->second;

  FuncId id(funcs.size());
  funcs.push_back(FuncDef(id, usr));
  usr_to_func_id[usr] = id;
  return id;
}
VarId ParsingDatabase::ToVarId(const std::string& usr) {
  auto it = usr_to_var_id.find(usr);
  if (it != usr_to_var_id.end())
    return it->second;

  VarId id(vars.size());
  vars.push_back(VarDef(id, usr));
  usr_to_var_id[usr] = id;
  return id;
}

TypeId ParsingDatabase::ToTypeId(const CXCursor& cursor) {
  return ToTypeId(clang::Cursor(cursor).get_usr());
}

FuncId ParsingDatabase::ToFuncId(const CXCursor& cursor) {
  return ToFuncId(clang::Cursor(cursor).get_usr());
}

VarId ParsingDatabase::ToVarId(const CXCursor& cursor) {
  return ToVarId(clang::Cursor(cursor).get_usr());
}


TypeDef* ParsingDatabase::Resolve(TypeId id) {
  return &types[id.local_id];
}
FuncDef* ParsingDatabase::Resolve(FuncId id) {
  return &funcs[id.local_id];
}
VarDef* ParsingDatabase::Resolve(VarId id) {
  return &vars[id.local_id];
}


using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;

void Write(Writer& writer, const char* key, Location location) {
  if (key) writer.Key(key);
  std::string s = location.ToString();
  writer.String(s.c_str());
}

void Write(Writer& writer, const char* key, std::optional<Location> location) {
  if (location) {
    Write(writer, key, location.value());
  }
  //else {
  //  if (key) writer.Key(key);
  //  writer.Null();
  //}
}

void Write(Writer& writer, const char* key, const std::vector<Location>& locs) {
  if (locs.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.StartArray();
  for (const Location& loc : locs)
    Write(writer, nullptr, loc);
  writer.EndArray();
}

template<typename T>
void Write(Writer& writer, const char* key, LocalId<T> id) {
  if (key) writer.Key(key);
  writer.Uint64(id.local_id);
}

template<typename T>
void Write(Writer& writer, const char* key, std::optional<LocalId<T>> id) {
  if (id) {
    Write(writer, key, id.value());
  }
  //else {
  //  if (key) writer.Key(key);
  //  writer.Null();
  //}
}

template<typename T>
void Write(Writer& writer, const char* key, const std::vector<LocalId<T>>& ids) {
  if (ids.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.StartArray();
  for (LocalId<T> id : ids)
    Write(writer, nullptr, id);
  writer.EndArray();
}

template<typename T>
void Write(Writer& writer, const char* key, Ref<T> ref) {
  if (key) writer.Key(key);
  std::string s = std::to_string(ref.id.local_id) + "@" + ref.loc.ToString();
  writer.String(s.c_str());
}

template<typename T>
void Write(Writer& writer, const char* key, const std::vector<Ref<T>>& refs) {
  if (refs.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.StartArray();
  for (Ref<T> ref : refs)
    Write(writer, nullptr, ref);
  writer.EndArray();
}

void Write(Writer& writer, const char* key, const std::string& value) {
  if (value.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.String(value.c_str());
}

void Write(Writer& writer, const char* key, uint64_t value) {
  if (key) writer.Key(key);
  writer.Uint64(value);
}

std::string ParsingDatabase::ToString() {
  auto it = usr_to_type_id.find("");
  if (it != usr_to_type_id.end()) {
    Resolve(it->second)->short_name = "<fundamental>";
    assert(Resolve(it->second)->uses.size() == 0);
  }

#define WRITE(name) Write(writer, #name, def.name)

  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  writer.SetFormatOptions(
    rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  writer.StartObject();

  // Types
  writer.Key("types");
  writer.StartArray();
  for (TypeDef& def : types) {
    writer.StartObject();
    WRITE(id);
    WRITE(usr);
    WRITE(short_name);
    WRITE(qualified_name);
    WRITE(definition);
    WRITE(alias_of);
    WRITE(parents);
    WRITE(derived);
    WRITE(types);
    WRITE(funcs);
    WRITE(vars);
    WRITE(uses);
    writer.EndObject();
  }
  writer.EndArray();

  // Functions
  writer.Key("functions");
  writer.StartArray();
  for (FuncDef& def : funcs) {
    writer.StartObject();
    WRITE(id);
    WRITE(usr);
    WRITE(short_name);
    WRITE(qualified_name);
    WRITE(declaration);
    WRITE(definition);
    WRITE(declaring_type);
    WRITE(base);
    WRITE(derived);
    WRITE(locals);
    WRITE(callers);
    WRITE(callees);
    WRITE(uses);
    writer.EndObject();
  }
  writer.EndArray();

  // Variables
  writer.Key("variables");
  writer.StartArray();
  for (VarDef& def : vars) {
    writer.StartObject();
    WRITE(id);
    WRITE(usr);
    WRITE(short_name);
    WRITE(qualified_name);
    WRITE(declaration);
    WRITE(definition);
    WRITE(variable_type);
    WRITE(declaring_type);
    WRITE(uses);
    writer.EndObject();
  }
  writer.EndArray();

  writer.EndObject();

  return output.GetString();

#undef WRITE
}

struct FileDef {
  uint64_t id;
  std::string path;
  std::vector<TypeDef> types;
  std::vector<FuncDef> funcs;
  std::vector<VarDef> vars;
};




template<typename T>
bool Contains(const std::vector<T>& vec, const T& element) {
  for (const T& entry : vec) {
    if (entry == element)
      return true;
  }
  return false;
}














int abortQuery(CXClientData client_data, void *reserved) {
  // 0 -> continue
  return 0;
}
void diagnostic(CXClientData client_data, CXDiagnosticSet, void *reserved) {}

CXIdxClientFile enteredMainFile(CXClientData client_data, CXFile mainFile, void *reserved) {
  return nullptr;
}

CXIdxClientFile ppIncludedFile(CXClientData client_data, const CXIdxIncludedFileInfo *) {
  return nullptr;
}

CXIdxClientASTFile importedASTFile(CXClientData client_data, const CXIdxImportedASTFileInfo *) {
  return nullptr;
}

CXIdxClientContainer startedTranslationUnit(CXClientData client_data, void *reserved) {
  return nullptr;
}





clang::VisiterResult DumpVisitor(clang::Cursor cursor, clang::Cursor parent, int* level) {
  for (int i = 0; i < *level; ++i)
    std::cout << "  ";
  std::cout << clang::ToString(cursor.get_kind()) << " " << cursor.get_spelling() << std::endl;

  *level += 1;
  cursor.VisitChildren(&DumpVisitor, level);
  *level -= 1;

  return clang::VisiterResult::Continue;
}

void Dump(clang::Cursor cursor) {
  int level = 0;
  cursor.VisitChildren(&DumpVisitor, &level);
}





struct FindChildOfKindParam {
  CXCursorKind target_kind;
  std::optional<clang::Cursor> result;

  FindChildOfKindParam(CXCursorKind target_kind) : target_kind(target_kind) {}
};

clang::VisiterResult FindChildOfKindVisitor(clang::Cursor cursor, clang::Cursor parent, FindChildOfKindParam* param) {
  if (cursor.get_kind() == param->target_kind) {
    param->result = cursor;
    return clang::VisiterResult::Break;
  }

  return clang::VisiterResult::Recurse;
}

std::optional<clang::Cursor> FindChildOfKind(clang::Cursor cursor, CXCursorKind kind) {
  FindChildOfKindParam param(kind);
  cursor.VisitChildren(&FindChildOfKindVisitor, &param);
  return param.result;
}





clang::VisiterResult FindTypeVisitor(clang::Cursor cursor, clang::Cursor parent, std::optional<clang::Cursor>* result) {
  switch (cursor.get_kind()) {
  case CXCursor_TypeRef:
  case CXCursor_TemplateRef:
    *result = cursor;
    return clang::VisiterResult::Break;
  }

  return clang::VisiterResult::Recurse;
}

std::optional<clang::Cursor> FindType(clang::Cursor cursor) {
  std::optional<clang::Cursor> result;
  cursor.VisitChildren(&FindTypeVisitor, &result);
  return result;
}







struct NamespaceHelper {
  std::unordered_map<std::string, std::string> container_usr_to_qualified_name;

  void RegisterQualifiedName(std::string usr, const CXIdxContainerInfo* container, std::string qualified_name) {
    if (container) {
      std::string container_usr = clang::Cursor(container->cursor).get_usr();
      auto it = container_usr_to_qualified_name.find(container_usr);
      if (it != container_usr_to_qualified_name.end()) {
        container_usr_to_qualified_name[usr] = it->second + qualified_name + "::";
        return;
      }
    }

    container_usr_to_qualified_name[usr] = qualified_name + "::";
  }

  std::string QualifiedName(const CXIdxContainerInfo* container, std::string unqualified_name) {
    if (container) {
      std::string container_usr = clang::Cursor(container->cursor).get_usr();
      auto it = container_usr_to_qualified_name.find(container_usr);
      if (it != container_usr_to_qualified_name.end())
        return it->second + unqualified_name;

      // Anonymous namespaces are not processed by indexDeclaration. If we
      // encounter one insert it into map.
      if (container->cursor.kind == CXCursor_Namespace) {
        assert(clang::Cursor(container->cursor).get_spelling() == "");
        container_usr_to_qualified_name[container_usr] = "::";
        return "::" + unqualified_name;
      }
    }
    return unqualified_name;
  }
};

struct IndexParam {
  ParsingDatabase* db;
  NamespaceHelper* ns;

  // Record the last type usage location we recorded. Clang will sometimes
  // visit the same expression twice so we wan't to avoid double-reporting
  // usage information for those locations.
  Location last_type_usage_location;
  Location last_func_usage_location;

  IndexParam(ParsingDatabase* db, NamespaceHelper* ns) : db(db), ns(ns) {}
};

/*
std::string GetNamespacePrefx(const CXIdxDeclInfo* decl) {
  const CXIdxContainerInfo* container = decl->lexicalContainer;
  while (container) {

  }
}
*/

bool IsTypeDefinition(const CXIdxContainerInfo* container) {
  if (!container)
    return false;

  switch (container->cursor.kind) {
  case CXCursor_EnumDecl:
  case CXCursor_UnionDecl:
  case CXCursor_StructDecl:
  case CXCursor_ClassDecl:
    return true;
  default:
    return false;
  }
}



struct VisitDeclForTypeUsageParam {
  ParsingDatabase* db;
  bool is_interesting;
  int has_processed_any = false;
  std::optional<clang::Cursor> previous_cursor;
  std::optional<TypeId> initial_type;

  VisitDeclForTypeUsageParam(ParsingDatabase* db, bool is_interesting)
    : db(db), is_interesting(is_interesting) {}
};

void VisitDeclForTypeUsageVisitorHandler(clang::Cursor cursor, VisitDeclForTypeUsageParam* param) {
  param->has_processed_any = true;
  ParsingDatabase* db = param->db;

  // TODO: Something in STL (type_traits)? reports an empty USR.
  std::string referenced_usr = cursor.get_referenced().get_usr();
  if (referenced_usr == "")
    return;

  TypeId ref_type_id = db->ToTypeId(referenced_usr);
  if (!param->initial_type)
    param->initial_type = ref_type_id;

  if (param->is_interesting) {
    TypeDef* ref_type_def = db->Resolve(ref_type_id);
    Location loc = db->file_db.Resolve(cursor, true /*interesting*/);
    ref_type_def->AddUsage(loc);
  }
}

clang::VisiterResult VisitDeclForTypeUsageVisitor(clang::Cursor cursor, clang::Cursor parent, VisitDeclForTypeUsageParam* param) {
  switch (cursor.get_kind()) {
  case CXCursor_TemplateRef:
  case CXCursor_TypeRef:
    if (param->previous_cursor) {
      VisitDeclForTypeUsageVisitorHandler(param->previous_cursor.value(), param);

      // This if is inside the above if because if there are multiple TypeRefs,
      // we always want to process the first one. If we did not always process
      // the first one, we cannot tell if there are more TypeRefs after it and
      // logic for fetching the return type breaks. This happens in ParmDecl
      // instances which only have one TypeRef child but are not interesting
      // usages.
      if (!param->is_interesting)
        return clang::VisiterResult::Break;
    }

    param->previous_cursor = cursor;
  }

  return clang::VisiterResult::Continue;
}

std::optional<TypeId> ResolveDeclToType(ParsingDatabase* db, clang::Cursor decl_cursor,
  bool is_interesting, const CXIdxContainerInfo* semantic_container,
  const CXIdxContainerInfo* lexical_container) {
  //
  // The general AST format for definitions follows this pattern:
  //
  //  template<typename A, typename B>
  //  struct Container;
  //
  //  struct S1;
  //  struct S2;
  //
  //  Container<Container<S1, S2>, S2> foo;
  //
  //  =>
  //
  //  VarDecl
  //    TemplateRef Container
  //    TemplateRef Container
  //    TypeRef struct S1
  //    TypeRef struct S2
  //    TypeRef struct S2
  //

  // We skip the last type reference for methods/variables which are defined
  // out-of-line w.r.t. the parent type.
  //
  //  S1* Foo::foo() {}
  // 
  // The above example looks like this in the AST:
  //
  //  CXXMethod foo
  //    TypeRef struct S1
  //    TypeRef class Foo
  //    CompoundStmt
  //      ...
  //
  //  The second TypeRef is an uninteresting usage.
  bool process_last_type_ref = true;
  if (IsTypeDefinition(semantic_container) && !IsTypeDefinition(lexical_container)) {
    assert(decl_cursor.is_definition());
    process_last_type_ref = false;
  }

  VisitDeclForTypeUsageParam param(db, is_interesting);
  decl_cursor.VisitChildren(&VisitDeclForTypeUsageVisitor, &param);

  // VisitDeclForTypeUsageVisitor guarantees that if there are multiple TypeRef
  // children, the first one will always be visited.
  if (param.previous_cursor && process_last_type_ref) {
    VisitDeclForTypeUsageVisitorHandler(param.previous_cursor.value(), &param);
  }
  else {
    // If we are not processing the last type ref, it *must* be a TypeRef (ie,
    // and not a TemplateRef).
    assert(!param.previous_cursor.has_value() ||
      param.previous_cursor.value().get_kind() == CXCursor_TypeRef);
  }

  return param.initial_type;
}


void indexDeclaration(CXClientData client_data, const CXIdxDeclInfo* decl) {
  IndexParam* param = static_cast<IndexParam*>(client_data);
  ParsingDatabase* db = param->db;
  NamespaceHelper* ns = param->ns;

  switch (decl->entityInfo->kind) {
  case CXIdxEntity_CXXNamespace:
  {
    ns->RegisterQualifiedName(decl->entityInfo->USR, decl->semanticContainer, decl->entityInfo->name);
    break;
  }

  case CXIdxEntity_EnumConstant:
  case CXIdxEntity_Field:
  case CXIdxEntity_Variable:
  case CXIdxEntity_CXXStaticVariable:
  {
    clang::Cursor decl_cursor = decl->cursor;
    VarId var_id = db->ToVarId(decl->entityInfo->USR);
    VarDef* var_def = db->Resolve(var_id);

    // TODO: Eventually run with this if. Right now I want to iron out bugs this may shadow.
    // TODO: Verify this gets called multiple times
    //if (!decl->isRedeclaration) {
    var_def->short_name = decl->entityInfo->name;
    var_def->qualified_name = ns->QualifiedName(decl->semanticContainer, var_def->short_name);
    //}

    Location decl_loc = db->file_db.Resolve(decl->loc, false /*interesting*/);
    if (decl->isDefinition)
      var_def->definition = decl_loc;
    else
      var_def->declaration = decl_loc;
    var_def->uses.push_back(decl_loc);


    // Declaring variable type information. Note that we do not insert an
    // interesting reference for parameter declarations - that is handled when
    // the function declaration is encountered since we won't receive ParmDecl
    // declarations for unnamed parameters.
    std::optional<TypeId> var_type = ResolveDeclToType(db, decl_cursor, decl_cursor.get_kind() != CXCursor_ParmDecl /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);
    if (var_type.has_value())
      var_def->variable_type = var_type.value();


    if (decl->isDefinition && IsTypeDefinition(decl->semanticContainer)) {
      TypeId declaring_type_id = db->ToTypeId(decl->semanticContainer->cursor);
      TypeDef* declaring_type_def = db->Resolve(declaring_type_id);
      var_def->declaring_type = declaring_type_id;
      declaring_type_def->vars.push_back(var_id);
    }

    break;
  }

  case CXIdxEntity_Function:
  case CXIdxEntity_CXXConstructor:
  case CXIdxEntity_CXXDestructor:
  case CXIdxEntity_CXXInstanceMethod:
  case CXIdxEntity_CXXStaticMethod:
  {
    clang::Cursor decl_cursor = decl->cursor;
    FuncId func_id = db->ToFuncId(decl->entityInfo->USR);
    FuncDef* func_def = db->Resolve(func_id);

    // TODO: Eventually run with this if. Right now I want to iron out bugs this may shadow.
    //if (!decl->isRedeclaration) {
    func_def->short_name = decl->entityInfo->name;
    func_def->qualified_name = ns->QualifiedName(decl->semanticContainer, func_def->short_name);
    //}

    Location decl_loc = db->file_db.Resolve(decl->loc, false /*interesting*/);
    if (decl->isDefinition)
      func_def->definition = decl_loc;
    else
      func_def->declaration = decl_loc;
    func_def->uses.push_back(decl_loc);

    bool is_pure_virtual = clang_CXXMethod_isPureVirtual(decl->cursor);
    bool is_ctor_or_dtor = decl->entityInfo->kind == CXIdxEntity_CXXConstructor || decl->entityInfo->kind == CXIdxEntity_CXXDestructor;
    //bool process_declaring_type = is_pure_virtual || is_ctor_or_dtor;

    // Add function usage information. We only want to do it once per
    // definition/declaration. Do it on definition since there should only ever
    // be one of those in the entire program.
    if (IsTypeDefinition(decl->semanticContainer)) {
      TypeId declaring_type_id = db->ToTypeId(decl->semanticContainer->cursor);
      TypeDef* declaring_type_def = db->Resolve(declaring_type_id);
      func_def->declaring_type = declaring_type_id;

      // Mark a type reference at the ctor/dtor location.
      // TODO: Should it be interesting?
      if (is_ctor_or_dtor) {
        Location type_usage_loc = decl_loc;
        declaring_type_def->AddUsage(type_usage_loc);
      }

      // Register function in declaring type if it hasn't been registered yet.
      if (!Contains(declaring_type_def->funcs, func_id))
        declaring_type_def->funcs.push_back(func_id);
    }



    // We don't actually need to know the return type, but we need to mark it
    // as an interesting usage.
    ResolveDeclToType(db, decl_cursor, true /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);

    //TypeResolution ret_type = ResolveToType(db, decl_cursor.get_type().get_return_type());
    //if (ret_type.resolved_type)
    //  AddInterestingUsageToType(db, ret_type, FindLocationOfTypeSpecifier(decl_cursor));

    if (decl->isDefinition || is_pure_virtual) {
      // Mark type usage for parameters as interesting. We handle this here
      // instead of inside var declaration because clang will not emit a var
      // declaration for an unnamed parameter, but we still want to mark the
      // usage as interesting.
      // TODO: Do a similar thing for function decl parameter usages. Mark
      //       prototype params as interesting type usages but also relate mark
      //       them as as usages on the primary variable - requires USR to be
      //       the same. We can work around it by declaring which variables a
      //       parameter has declared and update the USR in the definition.
      clang::Cursor cursor = decl->cursor;
      for (clang::Cursor arg : cursor.get_arguments()) {
        switch (arg.get_kind()) {
        case CXCursor_ParmDecl:
          // We don't need to know the arg type, but we do want to mark it as
          // an interesting usage.
          ResolveDeclToType(db, arg, true /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);

          //TypeResolution arg_type = ResolveToType(db, arg.get_type());
          //if (arg_type.resolved_type)
          //  AddInterestingUsageToType(db, arg_type, FindLocationOfTypeSpecifier(arg));
          break;
        }
      }


      // Process inheritance.
      //void clang_getOverriddenCursors(CXCursor cursor, CXCursor **overridden, unsigned *num_overridden);
      //void clang_disposeOverriddenCursors(CXCursor *overridden);
      if (clang_CXXMethod_isVirtual(decl->cursor)) {
        CXCursor* overridden;
        unsigned int num_overridden;
        clang_getOverriddenCursors(decl->cursor, &overridden, &num_overridden);

        // TODO: How to handle multiple parent overrides??
        for (unsigned int i = 0; i < num_overridden; ++i) {
          clang::Cursor parent = overridden[i];
          FuncId parent_id = db->ToFuncId(parent.get_usr());
          FuncDef* parent_def = db->Resolve(parent_id);
          func_def = db->Resolve(func_id); // ToFuncId invalidated func_def

          func_def->base = parent_id;
          parent_def->derived.push_back(func_id);
        }

        clang_disposeOverriddenCursors(overridden);
      }
    }

    /*
    std::optional<FuncId> base;
    std::vector<FuncId> derived;
    std::vector<VarId> locals;
    std::vector<FuncRef> callers;
    std::vector<FuncRef> callees;
    std::vector<Location> uses;
    */
    break;
  }

  case CXIdxEntity_Typedef:
  case CXIdxEntity_CXXTypeAlias:
  {
    std::optional<TypeId> alias_of = ResolveDeclToType(db, decl->cursor, true /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);

    TypeId type_id = db->ToTypeId(decl->entityInfo->USR);
    TypeDef* type_def = db->Resolve(type_id);

    if (alias_of)
      type_def->alias_of = alias_of.value();

    type_def->short_name = decl->entityInfo->name;
    type_def->qualified_name = ns->QualifiedName(decl->semanticContainer, type_def->short_name);

    Location decl_loc = db->file_db.Resolve(decl->loc, true /*interesting*/);
    type_def->definition = decl_loc.WithInteresting(false);
    type_def->AddUsage(decl_loc);
    break;
  }

  case CXIdxEntity_Enum:
  case CXIdxEntity_Union:
  case CXIdxEntity_Struct:
  case CXIdxEntity_CXXClass:
  {
    TypeId type_id = db->ToTypeId(decl->entityInfo->USR);
    TypeDef* type_def = db->Resolve(type_id);

    // TODO: Eventually run with this if. Right now I want to iron out bugs this may shadow.
    // TODO: For type section, verify if this ever runs for non definitions?
    //if (!decl->isRedeclaration) {

    // name can be null in an anonymous struct (see tests/types/anonymous_struct.cc).
    if (decl->entityInfo->name) {
      ns->RegisterQualifiedName(decl->entityInfo->USR, decl->semanticContainer, decl->entityInfo->name);
      type_def->short_name = decl->entityInfo->name;
    }
    else {
      type_def->short_name = "<anonymous>";
    }

    type_def->qualified_name = ns->QualifiedName(decl->semanticContainer, type_def->short_name);

    // }

    assert(decl->isDefinition);
    Location decl_loc = db->file_db.Resolve(decl->loc, true /*interesting*/);
    type_def->definition = decl_loc.WithInteresting(false);
    type_def->AddUsage(decl_loc);

    //type_def->alias_of
    //type_def->funcs
    //type_def->types
    //type_def->uses
    //type_def->vars

    // Add type-level inheritance information.
    CXIdxCXXClassDeclInfo const* class_info = clang_index_getCXXClassDeclInfo(decl);
    if (class_info) {
      for (unsigned int i = 0; i < class_info->numBases; ++i) {
        const CXIdxBaseClassInfo* base_class = class_info->bases[i];

        std::optional<TypeId> parent_type_id = ResolveDeclToType(db, base_class->cursor, true /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);
        TypeDef* type_def = db->Resolve(type_id); // type_def ptr could be invalidated by ResolveDeclToType.
        if (parent_type_id) {
          TypeDef* parent_type_def = db->Resolve(parent_type_id.value());
          parent_type_def->derived.push_back(type_id);
          type_def->parents.push_back(parent_type_id.value());
        }
      }
    }
    break;
  }

  default:
    std::cout << "!! Unhandled indexDeclaration:     " << clang::Cursor(decl->cursor).ToString() << " at " << db->file_db.Resolve(decl->loc, false /*interesting*/).ToString() << std::endl;
    std::cout << "     entityInfo->kind  = " << decl->entityInfo->kind << std::endl;
    std::cout << "     entityInfo->USR   = " << decl->entityInfo->USR << std::endl;
    if (decl->declAsContainer)
      std::cout << "     declAsContainer   = " << clang::Cursor(decl->declAsContainer->cursor).ToString() << std::endl;
    if (decl->semanticContainer)
      std::cout << "     semanticContainer = " << clang::Cursor(decl->semanticContainer->cursor).ToString() << std::endl;
    if (decl->lexicalContainer)
      std::cout << "     lexicalContainer  = " << clang::Cursor(decl->lexicalContainer->cursor).get_usr() << std::endl;
    break;
  }
}

bool IsFunction(CXCursorKind kind) {
  switch (kind) {
  case CXCursor_CXXMethod:
  case CXCursor_FunctionDecl:
    return true;
  }

  return false;
}

void indexEntityReference(CXClientData client_data, const CXIdxEntityRefInfo* ref) {
  IndexParam* param = static_cast<IndexParam*>(client_data);
  ParsingDatabase* db = param->db;
  clang::Cursor cursor(ref->cursor);

  switch (ref->referencedEntity->kind) {
  case CXIdxEntity_EnumConstant:
  case CXIdxEntity_CXXStaticVariable:
  case CXIdxEntity_Variable:
  case CXIdxEntity_Field:
  {
    VarId var_id = db->ToVarId(ref->referencedEntity->cursor);
    VarDef* var_def = db->Resolve(var_id);
    var_def->uses.push_back(db->file_db.Resolve(ref->loc, false /*interesting*/));
    break;
  }

  case CXIdxEntity_CXXStaticMethod:
  case CXIdxEntity_CXXInstanceMethod:
  case CXIdxEntity_Function:
  case CXIdxEntity_CXXConstructor:
  case CXIdxEntity_CXXDestructor:
  {
    // TODO: Redirect container to constructor for the following example, ie,
    //       we should be inserting an outgoing function call from the Foo
    //       ctor.
    //
    //  int Gen() { return 5; }
    //  class Foo {
    //    int x = Gen();
    //  }

    // Don't report duplicate usages.
    // TODO: search full history?
    Location loc = db->file_db.Resolve(ref->loc, false /*interesting*/);
    if (param->last_func_usage_location == loc) break;
    param->last_func_usage_location = loc;

    // Note: be careful, calling db->ToFuncId invalidates the FuncDef* ptrs.
    FuncId called_id = db->ToFuncId(ref->referencedEntity->USR);
    if (IsFunction(ref->container->cursor.kind)) {
      FuncId caller_id = db->ToFuncId(ref->container->cursor);
      FuncDef* caller_def = db->Resolve(caller_id);
      FuncDef* called_def = db->Resolve(called_id);

      caller_def->callees.push_back(FuncRef(called_id, loc));
      called_def->callers.push_back(FuncRef(caller_id, loc));
      called_def->uses.push_back(loc);
    }
    else {
      FuncDef* called_def = db->Resolve(called_id);
      called_def->uses.push_back(loc);
    }

    // For constructor/destructor, also add a usage against the type. Clang
    // will insert and visit implicit constructor references, so we also check
    // the location of the ctor call compared to the parent call. If they are
    // the same, this is most likely an implicit ctors.
    clang::Cursor ref_cursor = ref->cursor;
    if (ref->referencedEntity->kind == CXIdxEntity_CXXConstructor ||
        ref->referencedEntity->kind == CXIdxEntity_CXXDestructor) {

      Location parent_loc = db->file_db.Resolve(ref->parentEntity->cursor, true /*interesting*/);
      Location our_loc = db->file_db.Resolve(ref->loc, true /*is_interesting*/);
      if (!parent_loc.IsEqualTo(our_loc)) {
        FuncDef* called_def = db->Resolve(called_id);
        assert(called_def->declaring_type.has_value());
        TypeDef* type_def = db->Resolve(called_def->declaring_type.value());
        type_def->AddUsage(our_loc);
      }
    }
    break;
  }

  case CXIdxEntity_Typedef:
  case CXIdxEntity_CXXTypeAlias:
  case CXIdxEntity_Enum:
  case CXIdxEntity_Union:
  case CXIdxEntity_Struct:
  case CXIdxEntity_CXXClass:
  {
    TypeId referenced_id = db->ToTypeId(ref->referencedEntity->USR);
    TypeDef* referenced_def = db->Resolve(referenced_id);

    //
    // The following will generate two TypeRefs to Foo, both located at the
    // same spot (line 3, column 3). One of the parents will be set to
    // CXIdxEntity_Variable, the other will be CXIdxEntity_Function. There does
    // not appear to be a good way to disambiguate these references, as using
    // parent type alone breaks other indexing tasks.
    //
    // To work around this, we check to see if the usage location has been
    // inserted into all_uses previously.
    //
    //  struct Foo {};
    //  void Make() {
    //    Foo f;
    //  }
    //
    referenced_def->AddUsage(db->file_db.Resolve(ref->loc, false /*interesting*/));
    break;
  }

  default:
    std::cout << "!! Unhandled indexEntityReference: " << cursor.ToString() << " at " << db->file_db.Resolve(ref->loc, false /*interesting*/).ToString() << std::endl;
    std::cout << "     ref->referencedEntity->kind = " << ref->referencedEntity->kind << std::endl;
    if (ref->parentEntity)
      std::cout << "     ref->parentEntity->kind = " << ref->parentEntity->kind << std::endl;
    std::cout << "     ref->loc          = " << db->file_db.Resolve(ref->loc, false /*interesting*/).ToString() << std::endl;
    std::cout << "     ref->kind         = " << ref->kind << std::endl;
    if (ref->parentEntity)
      std::cout << "     parentEntity      = " << clang::Cursor(ref->parentEntity->cursor).ToString() << std::endl;
    if (ref->referencedEntity)
      std::cout << "     referencedEntity  = " << clang::Cursor(ref->referencedEntity->cursor).ToString() << std::endl;
    if (ref->container)
      std::cout << "     container         = " << clang::Cursor(ref->container->cursor).ToString() << std::endl;
    break;
  }
}

static bool DUMP_AST = true;


ParsingDatabase Parse(std::string filename) {
  std::vector<std::string> args;

  clang::Index index(0 /*excludeDeclarationsFromPCH*/, 0 /*displayDiagnostics*/);
  clang::TranslationUnit tu(index, filename, args);

  if (DUMP_AST)
    Dump(tu.document_cursor());

  CXIndexAction index_action = clang_IndexAction_create(index.cx_index);

  IndexerCallbacks callbacks[] = {
    { &abortQuery, &diagnostic, &enteredMainFile, &ppIncludedFile, &importedASTFile, &startedTranslationUnit, &indexDeclaration, &indexEntityReference }
    /*
    callbacks.abortQuery = &abortQuery;
    callbacks.diagnostic = &diagnostic;
    callbacks.enteredMainFile = &enteredMainFile;
    callbacks.ppIncludedFile = &ppIncludedFile;
    callbacks.importedASTFile = &importedASTFile;
    callbacks.startedTranslationUnit = &startedTranslationUnit;
    callbacks.indexDeclaration = &indexDeclaration;
    callbacks.indexEntityReference = &indexEntityReference;
    */
  };

  ParsingDatabase db;
  NamespaceHelper ns;
  IndexParam param(&db, &ns);
  clang_indexTranslationUnit(index_action, &param, callbacks, sizeof(callbacks),
    CXIndexOpt_IndexFunctionLocalSymbols, tu.cx_tu);

  clang_IndexAction_dispose(index_action);

  return db;
}


template<typename T>
bool AreEqual(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size())
    return false;

  for (int i = 0; i < a.size(); ++i) {
    if (a[i] != b[i])
      return false;
  }

  return true;
}

void Write(const std::vector<std::string>& strs) {
  for (const std::string& str : strs) {
    std::cout << str << std::endl;
  }
}


std::string ToString(const rapidjson::Document& document) {
  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  writer.SetFormatOptions(
    rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  buffer.Clear();
  document.Accept(writer);
  return buffer.GetString();
}

std::vector<std::string> split_string(const std::string& str, const std::string& delimiter) {
  // http://stackoverflow.com/a/13172514
  std::vector<std::string> strings;

  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while ((pos = str.find(delimiter, prev)) != std::string::npos) {
    strings.push_back(str.substr(prev, pos - prev));
    prev = pos + 1;
  }

  // To get the last substring (or only, if delimiter is not found)
  strings.push_back(str.substr(prev));

  return strings;
}


void DiffDocuments(rapidjson::Document& expected, rapidjson::Document& actual) {
  std::vector<std::string> actual_output;
  {
    std::string buffer = ToString(actual);
    actual_output = split_string(buffer, "\n");
  }

  std::vector<std::string> expected_output;
  {
    std::string buffer = ToString(expected);
    expected_output = split_string(buffer, "\n");
  }

  int len = std::min(actual_output.size(), expected_output.size());
  for (int i = 0; i < len; ++i) {
    if (actual_output[i] != expected_output[i]) {
      std::cout << "Line " << i << " differs:" << std::endl;
      std::cout << "  expected: " << expected_output[i] << std::endl;
      std::cout << "  actual:   " << actual_output[i] << std::endl;
    }
  }

  if (actual_output.size() > len) {
    std::cout << "Additional output in actual:" << std::endl;
    for (int i = len; i < actual_output.size(); ++i)
      std::cout << "  " << actual_output[i] << std::endl;
  }

  if (expected_output.size() > len) {
    std::cout << "Additional output in expected:" << std::endl;
    for (int i = len; i < expected_output.size(); ++i)
      std::cout << "  " << expected_output[i] << std::endl;
  }
}

void WriteToFile(const std::string& filename, const std::string& content) {
  std::ofstream file(filename);
  file << content;
}

int main(int argc, char** argv) {
  /*
  ParsingDatabase db = Parse("tests/vars/function_local.cc");
  std::cout << std::endl << "== Database ==" << std::endl;
  std::cout << db.ToString();
  std::cin.get();
  return 0;
  */

  DUMP_AST = false;

  for (std::string path : GetFilesInFolder("tests")) {
    //if (path != "tests/declaration_vs_definition/class_member_static.cc") continue;
    //if (path != "tests/enums/enum_class_decl.cc") continue;
    //if (path != "tests/constructors/constructor.cc") continue;
    //if (path == "tests/constructors/destructor.cc") continue;
    //if (path == "tests/usage/func_usage_call_method.cc") continue;
    //if (path != "tests/usage/type_usage_as_template_parameter.cc") continue;
    //if (path != "tests/usage/type_usage_as_template_parameter_complex.cc") continue;
    //if (path != "tests/usage/type_usage_as_template_parameter_simple.cc") continue;
    //if (path != "tests/usage/type_usage_typedef_and_using.cc") continue;
    //if (path != "tests/usage/type_usage_declare_local.cc") continue;
    //if (path == "tests/usage/type_usage_typedef_and_using_template.cc") continue;
    //if (path != "tests/usage/func_usage_addr_method.cc") continue;
    //if (path != "tests/usage/type_usage_typedef_and_using.cc") continue;
    //if (path != "tests/usage/usage_inside_of_call.cc") continue;
    if (path != "tests/foobar.cc") continue;
    //if (path != "tests/types/anonymous_struct.cc") continue;

    // Parse expected output from the test, parse it into JSON document.
    std::string expected_output;
    ParseTestExpectation(path, &expected_output);
    rapidjson::Document expected;
    expected.Parse(expected_output.c_str());

    // Run test.
    std::cout << "[START] " << path << std::endl;
    ParsingDatabase db = Parse(path);
    std::string actual_output = db.ToString();
    
    WriteToFile("output.json", actual_output);
    break;

    rapidjson::Document actual;
    actual.Parse(actual_output.c_str());

    if (actual == expected) {
      std::cout << "[PASSED] " << path << std::endl;
    }
    else {
      std::cout << "[FAILED] " << path << std::endl;
      std::cout << "Expected output for " << path << ":" << std::endl;
      std::cout << expected_output;
      std::cout << "Actual output for " << path << ":" << std::endl;
      std::cout << actual_output;
      std::cout << std::endl;
      std::cout << std::endl;
      DiffDocuments(expected, actual);
      break;
    }
  }

  //std::cin.get();
  return 0;
}

// TODO: ctor/dtor, copy ctor