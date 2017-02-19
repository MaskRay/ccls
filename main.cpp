#include <algorithm>
#include <optional>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <fstream>
#include <unordered_map>

#include "libclangmm/clangmm.h"
#include "libclangmm/Utility.h"

#include "utils.h"

#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>

//#include <clang-c\Index.h>


// While indexing, we should refer to symbols by USR. When joining into the db, we can have optimized access.

struct TypeDef;
struct FuncDef;
struct VarDef;

/*
template<typename T>
struct Id {
  uint64_t file_id;
  uint64_t local_id;

  Id() : file_id(0), local_id(0) {} // Needed for containers. Do not use directly.
  Id(uint64_t file_id, uint64_t local_id)
    : file_id(file_id), local_id(local_id) {}
};
*/

template<typename T>
struct LocalId {
  uint64_t local_id;

  LocalId() : local_id(0) {} // Needed for containers. Do not use directly.
  explicit LocalId(uint64_t local_id) : local_id(local_id) {}
};
using TypeId = LocalId<TypeDef>;
using FuncId = LocalId<FuncDef>;
using VarId = LocalId<VarDef>;


template<typename T>
struct Ref {
  LocalId<T> id;
  clang::SourceLocation loc;

  Ref(LocalId<T> id, clang::SourceLocation loc) : id(id), loc(loc) {}
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
  std::optional<clang::SourceLocation> declaration; // Forward decl.
  std::optional<clang::SourceLocation> definition;

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

  // Usages.
  std::vector<clang::SourceLocation> uses;

  TypeDef(TypeId id, const std::string& usr) : id(id), usr(usr) {
    assert(usr.size() > 0);
    //std::cout << "Creating type with usr " << usr << std::endl;
  }
};

struct FuncDef {
  // General metadata.
  FuncId id;
  std::string usr;
  std::string short_name;
  std::string qualified_name;
  std::optional<clang::SourceLocation> declaration;
  std::optional<clang::SourceLocation> definition;

  // Type which declares this one (ie, it is a method)
  std::optional<TypeId> declaring_type;
  // Method this method overrides.
  std::optional<FuncId> base;
  // Methods which directly override this one.
  std::vector<FuncId> derived;

  // Local variables defined in this function.
  std::vector<VarId> locals;

  // Functions which call this one.
  std::vector<FuncRef> callers;
  // Functions that this function calls.
  std::vector<FuncRef> callees;

  // Usages.
  std::vector<clang::SourceLocation> uses;

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
  std::optional<clang::SourceLocation> declaration;
  std::vector<clang::SourceLocation> initializations;

  // Type of the variable.
  std::optional<TypeId> variable_type;

  // Type which declares this one (ie, it is a method)
  std::optional<TypeId> declaring_type;

  // Usages.
  std::vector<clang::SourceLocation> uses;

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

  ParsingDatabase();

  TypeId ToTypeId(const std::string& usr);
  FuncId ToFuncId(const std::string& usr);
  VarId ToVarId(const std::string& usr);

  TypeDef* Resolve(TypeId id);
  FuncDef* Resolve(FuncId id);
  VarDef* Resolve(VarId id);

  std::string ToString();
};

ParsingDatabase::ParsingDatabase() {}

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

void Write(Writer& writer, const char* key, clang::SourceLocation location) {
  if (key) writer.Key(key);
  std::string s = location.ToString();
  writer.String(s.c_str());
}

void Write(Writer& writer, const char* key, std::optional<clang::SourceLocation> location) {
  if (location) {
    Write(writer, key, location.value());
  }
  //else {
  //  if (key) writer.Key(key);
  //  writer.Null();
  //}
}

void Write(Writer& writer, const char* key, const std::vector<clang::SourceLocation>& locs) {
  if (locs.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.StartArray();
  for (const clang::SourceLocation& loc : locs)
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
    WRITE(declaration);
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
    WRITE(initializations);
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


/*
struct Database {
  std::unordered_map<std::string, TypeId> usr_to_type_id;
  std::unordered_map<std::string, FuncId> usr_to_func_id;
  std::unordered_map<std::string, VarId> usr_to_var_id;

  std::vector<FileDef> files;

  TypeId ToTypeId(const std::string& usr);
  FuncId ToFuncId(const std::string& usr);
  VarId ToVarId(const std::string& usr);
};

TypeId Database::ToTypeId(const std::string& usr) {
  auto it = usr_to_type_id.find(usr);
  assert(it != usr_to_type_id.end() && "Usr is not registered");
  return it->second;
}
FuncId Database::ToFuncId(const std::string& usr) {
  auto it = usr_to_func_id.find(usr);
  assert(it != usr_to_func_id.end() && "Usr is not registered");
  return it->second;
}
VarId Database::ToVarId(const std::string& usr) {
  auto it = usr_to_var_id.find(usr);
  assert(it != usr_to_var_id.end() && "Usr is not registered");
  return it->second;
}

TypeDef* Resolve(FileDef* file, TypeId id) {
  assert(file->id == id.file_id);
  return &file->types[id.local_id];
}
FuncDef* Resolve(FileDef* file, FuncId id) {
  assert(file->id == id.file_id);
  return &file->funcs[id.local_id];
}
VarDef* Resolve(FileDef* file, VarId id) {
  assert(file->id == id.file_id);
  return &file->vars[id.local_id];
}

TypeDef* Resolve(Database* db, TypeId id) {
  return Resolve(&db->files[id.file_id], id);
}
FuncDef* Resolve(Database* db, FuncId id) {
  return Resolve(&db->files[id.file_id], id);
}
VarDef* Resolve(Database* db, VarId id) {
  return Resolve(&db->files[id.file_id], id);
}
*/

struct NamespaceStack {
  std::vector<std::string> stack;

  void Push(const std::string& ns);
  void Pop();
  std::string ComputeQualifiedName(
    ParsingDatabase* db, std::optional<TypeId> declaring_type, std::string short_name);

  static NamespaceStack kEmpty;
};
NamespaceStack NamespaceStack::kEmpty;

void NamespaceStack::Push(const std::string& ns) {
  stack.push_back(ns);
}

void NamespaceStack::Pop() {
  stack.pop_back();
}

std::string NamespaceStack::ComputeQualifiedName(
  ParsingDatabase* db, std::optional<TypeId> declaring_type, std::string short_name) {
  if (declaring_type) {
    TypeDef* def = db->Resolve(declaring_type.value());
    return def->qualified_name + "::" + short_name;
  }

  std::string result;
  for (const std::string& ns : stack)
    result += ns + "::";
  result += short_name;
  return result;
}









std::optional<TypeId> ResolveDeclaringType(CXCursorKind kind, ParsingDatabase* db, const clang::Cursor& cursor, std::optional<TypeId> declaring_type) {
  // Resolve the declaring type for out-of-line method definitions.
  if (!declaring_type) {
    clang::Cursor parent = cursor.get_semantic_parent();
    switch (parent.get_kind()) {
    case CXCursor_ClassDecl:
    case CXCursor_StructDecl:
      declaring_type = db->ToTypeId(parent.get_usr());
      break;
    }
  }

  // FieldDecl, etc must have a declaring type.
  assert(cursor.get_kind() != kind || declaring_type);

  return declaring_type;
}

// |func_id| is the function definition that is currently being processed.
void InsertReference(ParsingDatabase* db, std::optional<FuncId> func_id, clang::Cursor referencer) {
  clang::SourceLocation loc = referencer.get_source_location();
  clang::Cursor referenced = referencer.get_referenced();

  switch (referenced.get_kind()) {
  case CXCursor_Constructor:
  case CXCursor_Destructor:
  case CXCursor_CXXMethod:
  case CXCursor_FunctionDecl:
  {
    FuncId referenced_id = db->ToFuncId(referenced.get_usr());
    FuncDef* referenced_def = db->Resolve(referenced_id);

    if (func_id) {
      FuncDef* func_def = db->Resolve(func_id.value());
      func_def->callees.push_back(FuncRef(referenced_id, loc));
      referenced_def->callers.push_back(FuncRef(func_id.value(), loc));
    }

    referenced_def->uses.push_back(loc);
    break;
  }

  case CXCursor_ParmDecl:
  case CXCursor_FieldDecl:
  case CXCursor_VarDecl:
  {
    VarId referenced_id = db->ToVarId(referenced.get_usr());
    VarDef* referenced_def = db->Resolve(referenced_id);

    referenced_def->uses.push_back(loc);
    break;
  }
  default:
    std::cerr << "Unhandled reference from \"" << referencer.ToString()
      << "\" to \"" << referenced.ToString() << "\"" << std::endl;
    break;
  }
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



// TODO: See if we can improve type usage reporting, for example
//  void foo() {
//    Foo x;
//  }
// The usage on |Foo| will be reported at the |x| variable location. We should
// report it at the start of |Foo| instead. 

void InsertTypeUsageAtLocation(ParsingDatabase* db, clang::Type type, const clang::SourceLocation& location) {
  clang::Type raw_type = type.strip_qualifiers();

  std::string usr = raw_type.get_usr();
  if (usr == "")
    return;

  // Add a usage to the type of the variable.
  TypeId type_id = db->ToTypeId(raw_type.get_usr());
  db->Resolve(type_id)->uses.push_back(location);
}

struct VarDeclVisitorParam {
  ParsingDatabase* db;
  std::optional<FuncId> func_id;
  bool seen_type_ref = false;

  VarDeclVisitorParam(ParsingDatabase* db, std::optional<FuncId> func_id)
    : db(db), func_id(func_id) {}
};

// NOTE: This function does not process any of the definitions/etc defined
//       inside of the call initializing the variable. That should be handled
//       by the function-definition visitor!
clang::VisiterResult VarDeclVisitor(clang::Cursor cursor, clang::Cursor parent, VarDeclVisitorParam* param) {
  switch (cursor.get_kind()) {
  case CXCursor_TemplateRef:
    InsertTypeUsageAtLocation(param->db, cursor.get_referenced().get_type(), cursor.get_source_location());
    return clang::VisiterResult::Continue;

  case CXCursor_TypeRef:
    // This block of code will have two TypeRef nodes:
    //    Foo Foo::name = 3
    // We try to avoid the second reference here by only processing the first one.
    if (!param->seen_type_ref) {
      param->seen_type_ref = true;
      InsertTypeUsageAtLocation(param->db, cursor.get_referenced().get_type(), cursor.get_source_location());
    }
    return clang::VisiterResult::Continue;

  case CXCursor_UnexposedExpr:
  case CXCursor_UnaryOperator:
    return clang::VisiterResult::Continue;
    /*
    
  case CXCursor_CallExpr:
    // TODO: Add a test for parameters inside the call? We should probably recurse.
    InsertReference(param->db, param->func_id, cursor);
    return clang::VisiterResult::Continue;
    */
  default:
    std::cerr << "VarDeclVisitor unhandled " << cursor.ToString() << std::endl;
    return clang::VisiterResult::Continue;
  }
}

void HandleVarDecl(ParsingDatabase* db, NamespaceStack* ns, clang::Cursor var, std::optional<TypeId> declaring_type, std::optional<FuncId> func_id, bool declare_variable) {
  //Dump(var);

  // Add a usage to the type of the variable.
  //if (var.is_definition())
  //  InsertTypeUsageAtLocation(db, var.get_type(), var.get_source_location());

  // Add usage to types.
  VarDeclVisitorParam varDeclVisitorParam(db, func_id);
  var.VisitChildren(&VarDeclVisitor, &varDeclVisitorParam);
  
  if (!declare_variable)
    return;

  // Note: if there is no USR then there can be no declaring type, as all
  // member variables of a class must have a name. Only function parameters
  // can be nameless.
  std::string var_usr = var.get_usr();
  if (var_usr.size() == 0) {
    assert(var.get_kind() == CXCursor_ParmDecl);
    return;
  }

  VarId var_id = db->ToVarId(var_usr);
  VarDef* var_def = db->Resolve(var_id);

  declaring_type = ResolveDeclaringType(CXCursor_FieldDecl, db, var, declaring_type);
  if (declaring_type && !var_def->declaration) {
    // Note: If USR is null there can be no declaring type.
    db->Resolve(declaring_type.value())->vars.push_back(var_id);
    var_def->declaring_type = declaring_type;
  }

  // TODO: We could use RAII to verify we don't modify db while have a *Def
  //       instance alive.
  var_def->short_name = var.get_spelling();
  var_def->qualified_name =
    ns->ComputeQualifiedName(db, declaring_type, var_def->short_name);

  // We don't do any additional processing for non-definitions.
  if (!var.is_definition()) {
    var_def->declaration = var.get_source_location();
    return;
  }
  // If we're a definition and there hasn't been a forward decl, just assign
  // declaration location to definition location.
  else if (!var_def->declaration) {
    var_def->declaration = var.get_source_location();
  }

  // TODO: Figure out how to scan initializations properly. We probably need
  //       to scan for assignment statement, or definition+ctor.
  var_def->initializations.push_back(var.get_source_location());
  clang::Type var_type = var.get_type().strip_qualifiers();
  std::string var_type_usr = var.get_type().strip_qualifiers().get_usr();
  if (var_type_usr != "") {
    var_def->variable_type = db->ToTypeId(var_type_usr);
    /*
    for (clang::Type template_param_type : var_type.get_template_arguments()) {
      std::string usr = template_param_type.get_usr();
      if (usr == "")
        continue;

      //TypeId template_param_id = db->ToTypeId(usr);
      InsertTypeUsageAtLocation(db, template_param_type, var.get_source_location());

      //std::cout << template_param_type.get_usr() << std::endl;
    }*/
    //VarDeclVisitorParam varDeclVisitorParam(db, func_id);
    //var.VisitChildren(&VarDeclVisitor, &varDeclVisitorParam);
  }

}





// TODO: Should we declare variables on prototypes? ie,
//
//    foo(int* x);
//
// I'm inclined to say yes if we want a rename refactoring.


struct FuncDefinitionParam {
  ParsingDatabase* db;
  NamespaceStack* ns;
  FuncId func_id;
  bool is_definition;

  FuncDefinitionParam(ParsingDatabase* db, NamespaceStack* ns, FuncId func_id, bool is_definition)
    : db(db), ns(ns), func_id(func_id), is_definition(is_definition) {}
};

clang::VisiterResult VisitFuncDefinition(clang::Cursor cursor, clang::Cursor parent, FuncDefinitionParam* param) {
  //std::cout << "VistFuncDefinition got " << cursor.ToString() << std::endl;
  switch (cursor.get_kind()) {
    // TODO: Maybe we should default to recurse?
    /*
    case CXCursor_CompoundStmt:
    case CXCursor_DeclStmt:
    case CXCursor_CallExpr:
    case CXCursor_UnexposedExpr:
    case CXCursor_UnaryExpr:
      return clang::VisiterResult::Recurse;
    */

  case CXCursor_CallExpr:
    // The called element is handled by DeclRefExpr below.
    //InsertReference(param->db, param->func_id, cursor);
    return clang::VisiterResult::Recurse;

  case CXCursor_MemberRefExpr:
  case CXCursor_DeclRefExpr:
    InsertReference(param->db, param->func_id, cursor);
    return clang::VisiterResult::Recurse;

  case CXCursor_VarDecl:
  case CXCursor_ParmDecl:
    //std::cout << "!! Parsing var decl " << cursor.ToString() << std::endl;
    HandleVarDecl(param->db, param->ns, cursor, std::nullopt, param->func_id, param->is_definition);
    return clang::VisiterResult::Recurse;

  case CXCursor_ReturnStmt:
    return clang::VisiterResult::Recurse;

  default:
    //std::cerr << "Unhandled VisitFuncDefinition kind " << clang::ToString(cursor.get_kind()) << std::endl;
    return clang::VisiterResult::Recurse;
  }
}

void HandleFunc(ParsingDatabase* db, NamespaceStack* ns, clang::Cursor func, std::optional<TypeId> declaring_type) {
  // What this method must process:
  // - function declaration
  // - function definition
  // - method declaration
  // - method inline definition
  // - method definition

  // Resolve id before checking for is_definition so that we insert the
  // function into the db even if it is only a prototype. This is needed for
  // various file-level operations like outlining.
  FuncId func_id = db->ToFuncId(func.get_usr());

  // TODO: Consider skipping some of this processing if we've done it already
  //       (ie, parsed prototype, then parse definition).

  declaring_type =
    ResolveDeclaringType(CXCursor_CXXMethod, db, func, declaring_type);

  FuncDef* func_def = db->Resolve(func_id);

  func_def->short_name = func.get_spelling();
  func_def->qualified_name =
    ns->ComputeQualifiedName(db, declaring_type, func_def->short_name);

  if (declaring_type && !func_def->declaration) {
    db->Resolve(declaring_type.value())->funcs.push_back(func_id);
    func_def->declaring_type = declaring_type;
  }

  // Insert return type usage here instead of in the visitor. The only way to
  // do it in the visitor is to search for CXCursor_TypeRef, which does not
  // necessarily refer to the return type.
  // TODO: Is that the case? What about a top-level visitor? Would return type
  //       location be better?
  InsertTypeUsageAtLocation(db, func.get_type().get_return_type(), func.get_source_location());

  // Don't process definition/body for declarations.
  if (!func.is_definition()) {
    func_def->declaration = func.get_source_location();

    // We insert type references for arguments but don't use the normal visitor
    // because that will add a definition for the variable. These are not
    // "real" variables so we don't want to add definitions for them.

    // We navigate using cursor arguments so we can get location data.
    /*
    for (clang::Cursor arg : func.get_arguments()) {
      switch (arg.get_kind()) {
      case CXCursor_ParmDecl:
        InsertTypeUsageAtLocation(db, arg.get_type(), arg.get_source_location());
        break;
      }
    }
    */
  }

  if (func.is_definition())
    func_def->definition = func.get_source_location();

  FuncDefinitionParam funcDefinitionParam(db, &NamespaceStack::kEmpty, func_id, func.is_definition());
  func.VisitChildren(&VisitFuncDefinition, &funcDefinitionParam);
}








struct UsingParam {
  ParsingDatabase* db;
  TypeId active_type;

  UsingParam(ParsingDatabase* db, TypeId active_type)
    : db(db), active_type(active_type) {}
};

clang::VisiterResult VisitUsing(clang::Cursor cursor, clang::Cursor parent, UsingParam* param) {
  ParsingDatabase* db = param->db;

  switch (cursor.get_kind()) {
  case CXCursor_TypeRef:
  {
    TypeId source_type = db->ToTypeId(cursor.get_referenced().get_usr());
    db->Resolve(param->active_type)->alias_of = source_type;
    return clang::VisiterResult::Break;
  }
  default:
    std::cerr << "Unhandled VisitClassDecl kind " << clang::ToString(cursor.get_kind()) << std::endl;
    break;
  }

  return clang::VisiterResult::Continue;
}







struct ClassDeclParam {
  ParsingDatabase* db;
  NamespaceStack* ns;
  TypeId active_type;

  ClassDeclParam(ParsingDatabase* db, NamespaceStack* ns, TypeId active_type)
    : db(db), ns(ns), active_type(active_type) {}
};

clang::VisiterResult VisitClassDecl(clang::Cursor cursor, clang::Cursor parent, ClassDeclParam* param) {
  ParsingDatabase* db = param->db;

  switch (cursor.get_kind()) {
  case CXCursor_CXXAccessSpecifier:
    break;

  case CXCursor_Constructor:
  case CXCursor_Destructor:
  case CXCursor_CXXMethod:
    HandleFunc(param->db, param->ns, cursor, param->active_type);
    break;

  case CXCursor_FieldDecl:
  case CXCursor_VarDecl:
    HandleVarDecl(param->db, param->ns, cursor, param->active_type, std::nullopt, true /*declare_variable*/);
    break;

  default:
    std::cerr << "Unhandled VisitClassDecl kind " << clang::ToString(cursor.get_kind()) << std::endl;
    break;
  }

  return clang::VisiterResult::Continue;
}

void HandleClassDecl(clang::Cursor cursor, ParsingDatabase* db, NamespaceStack* ns, bool is_alias) {
  TypeId type_id = db->ToTypeId(cursor.get_usr());
  TypeDef* type_def = db->Resolve(type_id);

  type_def->short_name = cursor.get_spelling();
  // TODO: Support nested classes (pass in declaring type insteaad of nullopt!)
  type_def->qualified_name =
    ns->ComputeQualifiedName(db, std::nullopt, type_def->short_name);

  if (!cursor.is_definition()) {
    if (!type_def->declaration)
      type_def->declaration = cursor.get_source_location();
    return;
  }

  type_def->definition = cursor.get_source_location();

  if (is_alias) {
    UsingParam usingParam(db, type_id);
    cursor.VisitChildren(&VisitUsing, &usingParam);
  }
  else {
    ClassDeclParam classDeclParam(db, ns, type_id);
    cursor.VisitChildren(&VisitClassDecl, &classDeclParam);
  }
}








struct FileParam {
  ParsingDatabase* db;
  NamespaceStack* ns;

  FileParam(ParsingDatabase* db, NamespaceStack* ns) : db(db), ns(ns) {}
};

clang::VisiterResult VisitFile(clang::Cursor cursor, clang::Cursor parent, FileParam* param) {
  switch (cursor.get_kind()) {
  case CXCursor_Namespace:
    // For a namespace, visit the children of the namespace, but this time with
    // a pushed namespace stack.
    param->ns->Push(cursor.get_display_name());
    cursor.VisitChildren(&VisitFile, param);
    param->ns->Pop();
    break;

  case CXCursor_TypeAliasDecl:
  case CXCursor_TypedefDecl:
    HandleClassDecl(cursor, param->db, param->ns, true /*is_alias*/);
    break;

  case CXCursor_ClassTemplate:
  case CXCursor_StructDecl:
  case CXCursor_ClassDecl:
    // TODO: Cleanup Handle* param order.
    HandleClassDecl(cursor, param->db, param->ns, false /*is_alias*/);
    break;

  case CXCursor_CXXMethod:
  case CXCursor_FunctionDecl:
    HandleFunc(param->db, param->ns, cursor, std::nullopt);
    break;

  case CXCursor_VarDecl:
    HandleVarDecl(param->db, param->ns, cursor, std::nullopt, std::nullopt, true /*declare_variable*/);
    break;

  default:
    std::cerr << "Unhandled VisitFile kind " << clang::ToString(cursor.get_kind()) << std::endl;
    break;
  }

  return clang::VisiterResult::Continue;
}











ParsingDatabase Parse(std::string filename) {
  std::vector<std::string> args;

  clang::Index index(0 /*excludeDeclarationsFromPCH*/, 0 /*displayDiagnostics*/);
  clang::TranslationUnit tu(index, filename, args);

  std::cout << "Start document dump" << std::endl;
  Dump(tu.document_cursor());
  std::cout << "Done document dump" << std::endl << std::endl;

  ParsingDatabase db;
  NamespaceStack ns;
  FileParam fileParam(&db, &ns);
  tu.document_cursor().VisitChildren(&VisitFile, &fileParam);
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
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetFormatOptions(
      rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
    writer.SetIndent(' ', 2);

    buffer.Clear();
    actual.Accept(writer);
    actual_output = split_string(buffer.GetString(), "\n");
  }

  std::vector<std::string> expected_output;
  {
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetFormatOptions(
      rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
    writer.SetIndent(' ', 2);

    buffer.Clear();
    expected.Accept(writer);
    expected_output = split_string(buffer.GetString(), "\n");
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

int main(int argc, char** argv) {
  for (std::string path : GetFilesInFolder("tests")) {
    // TODO: Fix all existing tests.
    //if (path != "tests/constructors/constructor.cc") continue;
    //if (path != "tests/usage/func_usage_addr_func.cc") continue;
    //if (path != "tests/vars/class_static_member.cc") continue;

    // Parse expected output from the test, parse it into JSON document.
    std::string expected_output;
    ParseTestExpectation(path, &expected_output);
    rapidjson::Document expected;
    expected.Parse(expected_output.c_str());

    // Run test.
    std::cout << "[START] " << path << std::endl;
    ParsingDatabase db = Parse(path);
    std::string actual_output = db.ToString();
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

  std::cin.get();
  return 0;
}

// TODO: ctor/dtor, copy ctor