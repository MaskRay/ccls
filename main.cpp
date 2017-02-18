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

  // Immediate parent and immediate derived types.
  std::vector<TypeId> parents;
  std::vector<TypeId> derived;

  // Types, functions, and variables defined in this type.
  std::vector<TypeId> types;
  std::vector<FuncId> funcs;
  std::vector<VarId> vars;

  // Usages.
  std::vector<clang::SourceLocation> uses;

  TypeDef(TypeId id, const std::string& usr) : id(id), usr(usr) {}
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

  FuncDef(FuncId id, const std::string& usr) : id(id), usr(usr) {}
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

  VarDef(VarId id, const std::string& usr) : id(id), usr(usr) {}
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

  TypeId ToTypeId(const std::string& usr);
  FuncId ToFuncId(const std::string& usr);
  VarId ToVarId(const std::string& usr);

  TypeDef* Resolve(TypeId id);
  FuncDef* Resolve(FuncId id);
  VarDef* Resolve(VarId id);

  std::string ToString(bool for_test);
};

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

template<typename TWriter>
void WriteLocation(TWriter& writer, clang::SourceLocation location) {
  std::string s = location.ToString();
  writer.String(s.c_str());
}

template<typename TWriter>
void WriteLocation(TWriter& writer, std::optional<clang::SourceLocation> location) {
  if (location)
    WriteLocation(writer, location.value());
  else
    writer.Null();
}

template<typename TWriter, typename TId>
void WriteId(TWriter& writer, TId id) {
  writer.Uint64(id.local_id);
}

template<typename TWriter, typename TId>
void WriteId(TWriter& writer, std::optional<TId> id) {
  if (id)
    WriteId(writer, id.value());
  else
    writer.Null();
}

template<typename TWriter, typename TRef>
void WriteRef(TWriter& writer, TRef ref) {
  std::string s = std::to_string(ref.id.local_id) + "@" + ref.loc.ToString();
  writer.String(s.c_str());
}

template<typename TWriter, typename TId>
void WriteIdArray(TWriter& writer, const std::vector<TId>& ids) {
  writer.StartArray();
  for (TId id : ids)
    WriteId(writer, id);
  writer.EndArray();
}

template<typename TWriter, typename TRef>
void WriteRefArray(TWriter& writer, const std::vector<TRef>& refs) {
  writer.StartArray();
  for (TRef ref : refs)
    WriteRef(writer, ref);
  writer.EndArray();
}

template<typename TWriter>
void WriteLocationArray(TWriter& writer, const std::vector<clang::SourceLocation>& locs) {
  writer.StartArray();
  for (const clang::SourceLocation& loc : locs)
    WriteLocation(writer, loc);
  writer.EndArray();
}

std::string ParsingDatabase::ToString(bool for_test) {
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

    writer.String("id");
    writer.Uint64(def.id.local_id);

    if (!for_test) {
      writer.String("usr");
      writer.String(def.usr.c_str());
    }

    writer.String("short_name");
    writer.String(def.short_name.c_str());

    writer.String("qualified_name");
    writer.String(def.qualified_name.c_str());

    writer.String("declaration");
    WriteLocation(writer, def.declaration);

    if (!def.definition) {
      writer.EndObject();
      continue;
    }

    writer.String("definition");
    WriteLocation(writer, def.definition);

    writer.String("parents");
    WriteIdArray(writer, def.parents);

    writer.String("derived");
    WriteIdArray(writer, def.derived);

    writer.String("types");
    WriteIdArray(writer, def.types);

    writer.String("funcs");
    WriteIdArray(writer, def.funcs);

    writer.String("vars");
    WriteIdArray(writer, def.vars);

    writer.String("uses");
    WriteLocationArray(writer, def.uses);

    writer.EndObject();
  }
  writer.EndArray();

  // Functions
  writer.Key("functions");
  writer.StartArray();
  for (FuncDef& def : funcs) {
    writer.StartObject();

    writer.String("id");
    writer.Uint64(def.id.local_id);

    if (!for_test) {
      writer.String("usr");
      writer.String(def.usr.c_str());
    }

    writer.String("short_name");
    writer.String(def.short_name.c_str());

    writer.String("qualified_name");
    writer.String(def.qualified_name.c_str());

    writer.String("declaration");
    WriteLocation(writer, def.declaration);

    if (def.definition) {
      writer.String("definition");
      WriteLocation(writer, def.definition);
    }

    if (def.definition || def.declaring_type) {
      writer.String("declaring_type");
      WriteId(writer, def.declaring_type);
    }

    if (def.definition) {
      writer.String("base");
      WriteId(writer, def.base);

      writer.String("derived");
      WriteIdArray(writer, def.derived);

      writer.String("locals");
      WriteIdArray(writer, def.locals);

      writer.String("callers");
      WriteRefArray(writer, def.callers);

      writer.String("callees");
      WriteRefArray(writer, def.callees);

      writer.String("uses");
      WriteLocationArray(writer, def.uses);
    }

    writer.EndObject();
  }
  writer.EndArray();

  // Variables
  writer.Key("variables");
  writer.StartArray();
  for (VarDef& def : vars) {
    writer.StartObject();

    writer.String("id");
    writer.Uint64(def.id.local_id);

    if (!for_test) {
      writer.String("usr");
      writer.String(def.usr.c_str());
    }

    writer.String("short_name");
    writer.String(def.short_name.c_str());

    writer.String("qualified_name");
    writer.String(def.qualified_name.c_str());

    writer.String("declaration");
    WriteLocation(writer, def.declaration);

    if (def.initializations.size() == 0) {
      writer.EndObject();
      continue;
    }

    writer.String("initializations");
    WriteLocationArray(writer, def.initializations);

    writer.String("variable_type");
    WriteId(writer, def.variable_type);

    writer.String("declaring_type");
    WriteId(writer, def.declaring_type);

    writer.String("uses");
    WriteLocationArray(writer, def.uses);

    writer.EndObject();
  }
  writer.EndArray();

  writer.EndObject();

  return output.GetString();
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
  std::string ComputeQualifiedPrefix();

  static NamespaceStack kEmpty;
};
NamespaceStack NamespaceStack::kEmpty;

void NamespaceStack::Push(const std::string& ns) {
  stack.push_back(ns);
}

void NamespaceStack::Pop() {
  stack.pop_back();
}

std::string NamespaceStack::ComputeQualifiedPrefix() {
  std::string result;
  for (const std::string& ns : stack)
    result += ns + "::";
  return result;
}










std::optional<TypeId> ResolveDeclaringType(CXCursorKind kind, ParsingDatabase* db, const clang::Cursor& cursor, std::optional<TypeId> declaring_type) {
  // Resolve the declaring type for out-of-line method definitions.
  if (!declaring_type && cursor.get_kind() == kind) {
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















clang::VisiterResult DumpVisitor(clang::Cursor cursor, clang::Cursor parent, int* level) {
  for (int i = 0; i < *level; ++i)
    std::cout << "  ";
  std::cout << cursor.get_spelling() << " " << clang::ToString(cursor.get_kind()) << std::endl;

  *level += 1;
  cursor.VisitChildren(&DumpVisitor, level);
  *level -= 1;

  return clang::VisiterResult::Continue;
}

void Dump(clang::Cursor cursor) {
  int level = 0;
  cursor.VisitChildren(&DumpVisitor, &level);
}






void HandleVarDecl(ParsingDatabase* db, NamespaceStack* ns, clang::Cursor var, std::optional<TypeId> declaring_type) {

  Dump(var);

  VarId var_id = db->ToVarId(var.get_usr());

  declaring_type = ResolveDeclaringType(CXCursor_FieldDecl, db, var, declaring_type);

  // TODO: We could use RAII to verify we don't modify db while have a *Def
  //       instance alive.
  VarDef* var_def = db->Resolve(var_id);
  var_def->short_name = var.get_spelling();
  var_def->qualified_name = ns->ComputeQualifiedPrefix() + var_def->short_name;

  if (declaring_type && !var_def->declaration) {
    db->Resolve(declaring_type.value())->vars.push_back(var_id);
    var_def->declaring_type = declaring_type;
  }

  // We don't do any additional processing for non-definitions.
  if (!var.is_definition()) {
    var_def->declaration = var.get_source_location();
    return;
  }

  var_def->initializations.push_back(var.get_source_location());
  var_def->variable_type = db->ToTypeId(var.get_type().get_usr());
}










struct FuncDefinitionParam {
  ParsingDatabase* db;
  NamespaceStack* ns;
  FuncDefinitionParam(ParsingDatabase* db, NamespaceStack* ns)
    : db(db), ns(ns) {}
};

clang::VisiterResult VisitFuncDefinition(clang::Cursor cursor, clang::Cursor parent, FuncDefinitionParam* param) {
  //std::cout << "VistFunc got " << cursor.ToString() << std::endl;
  switch (cursor.get_kind()) {
  case CXCursor_CompoundStmt:
  case CXCursor_DeclStmt:
    return clang::VisiterResult::Recurse;

  case CXCursor_VarDecl:
  case CXCursor_ParmDecl:
    HandleVarDecl(param->db, param->ns, cursor, std::nullopt);
    return clang::VisiterResult::Continue;

  case CXCursor_ReturnStmt:
    return clang::VisiterResult::Continue;

  default:
    std::cerr << "Unhandled VisitFuncDefinition kind " << clang::ToString(cursor.get_kind()) << std::endl;
    return clang::VisiterResult::Continue;
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
  std::string type_name;
  if (declaring_type)
    type_name = db->Resolve(declaring_type.value())->short_name + "::";
  func_def->qualified_name =
    ns->ComputeQualifiedPrefix() + type_name + func_def->short_name;

  if (declaring_type && !func_def->declaration) {
    db->Resolve(declaring_type.value())->funcs.push_back(func_id);
    func_def->declaring_type = declaring_type;
  }

  // We don't do any additional processing for non-definitions.
  if (!func.is_definition()) {
    func_def->declaration = func.get_source_location();
    return;
  }

  func_def->definition = func.get_source_location();

  //std::cout << "!! Types: ";
  //for (clang::Cursor arg : func.get_arguments())
  //  std::cout << arg.ToString() << ", ";
  //std::cout << std::endl;

  //std::cout << func.get_usr() << ": Set qualified name to " << db->Resolve(id)->qualified_name;
  //std::cout << " IsDefinition? " << func.is_definition() << std::endl;

  //clang::Type func_type = func.get_type();
  //clang::Type return_type = func_type.get_return_type();
  //std::vector<clang::Type> argument_types = func_type.get_arguments();

  //auto argument_types = func.get_arguments();
  //clang::Type cursor_type = func.get_type();
  //clang::Type return_type_1 = func.get_type().get_result();
  //clang::Type return_type_2 = clang_getCursorResultType(func.cx_cursor);

  Dump(func);
  FuncDefinitionParam funcDefinitionParam(db, &NamespaceStack::kEmpty);
  func.VisitChildren(&VisitFuncDefinition, &funcDefinitionParam);

  //CXType return_type = clang_getResultType(func.get_type());
  //CXType_FunctionProto

  //std::cout << "!! HandleFunc " << func.get_type_description() << std::endl;
  //std::cout << "  comment: " << func.get_comments() << std::endl;
  //std::cout << "  spelling: " << func.get_spelling() << std::endl;
  //for (clang::Cursor argument : func.get_arguments())
  //  std::cout << "  arg: " << clang::ToString(argument.get_kind()) << " " << argument.get_spelling() << std::endl;
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
  case CXCursor_CXXMethod:
    HandleFunc(param->db, param->ns, cursor, param->active_type);
    break;

  case CXCursor_FieldDecl:
    HandleVarDecl(param->db, param->ns, cursor, param->active_type);
    break;

  default:
    std::cerr << "Unhandled VisitClassDecl kind " << clang::ToString(cursor.get_kind()) << std::endl;
    break;
  }

  return clang::VisiterResult::Continue;
}

void HandleClassDecl(clang::Cursor cursor, ParsingDatabase* db, NamespaceStack* ns) {
  TypeId id = db->ToTypeId(cursor.get_usr());
  TypeDef* def = db->Resolve(id);

  def->short_name = cursor.get_spelling();
  def->qualified_name = ns->ComputeQualifiedPrefix() + cursor.get_spelling();

  if (!cursor.is_definition()) {
    if (!def->declaration)
      def->declaration = cursor.get_source_location();
    return;
  }

  def->definition = cursor.get_source_location();

  ClassDeclParam classDeclParam(db, ns, id);
  cursor.VisitChildren(&VisitClassDecl, &classDeclParam);
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

  case CXCursor_ClassDecl:
    // TODO: Cleanup Handle* param order.
    HandleClassDecl(cursor, param->db, param->ns);
    break;

  case CXCursor_CXXMethod:
  case CXCursor_FunctionDecl:
    HandleFunc(param->db, param->ns, cursor, std::nullopt);
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

  //std::cout << "Start document dump" << std::endl;
  //Dump(tu.document_cursor());
  //std::cout << "Done document dump" << std::endl << std::endl;

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



int main(int argc, char** argv) {
  for (std::string path : GetFilesInFolder("tests")) {
    // TODO: Fix all existing tests.
    //if (path != "tests/vars/class_member.cc") continue;

    // Parse expected output from the test, parse it into JSON document.
    std::string expected_output;
    ParseTestExpectation(path, &expected_output);
    rapidjson::Document expected;
    expected.Parse(expected_output.c_str());

    // Run test.
    std::cout << "[START] " << path << std::endl;
    ParsingDatabase db = Parse(path);
    std::string actual_output = db.ToString(true /*for_test*/);
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
      break;
    }
  }

  std::cin.get();
  return 0;
}