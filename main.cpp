#include <optional>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <fstream>

#include "libclangmm\clangmm.h"
#include "libclangmm\Utility.h"

#include "utils.h"
//#include <clang-c\Index.h>


// While indexing, we should refer to symbols by USR. When joining into the db, we can have optimized access.

struct TypeDef;
struct FuncDef;
struct VarDef;


template<typename T>
struct Id {
  uint64_t file_id;
  uint64_t local_id;

  Id() : file_id(0), local_id(0) {} // Needed for containers. Do not use directly.
  Id(uint64_t file_id, uint64_t local_id)
    : file_id(file_id), local_id(local_id) {}
};
using TypeId = Id<TypeDef>;
using FuncId = Id<FuncDef>;
using VarId = Id<VarDef>;


template<typename T>
struct Ref {
  Id<T> id;
  clang::SourceLocation loc;
};
using TypeRef = Ref<TypeDef>;
using FuncRef = Ref<FuncDef>;
using VarRef = Ref<VarDef>;


struct TypeDef {
  TypeDef(TypeId id);

  // General metadata.
  TypeId id;
  std::string usr;
  std::string shortName;
  std::string qualifiedName;
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
};

TypeDef::TypeDef(TypeId id) : id(id) {}

struct FuncDef {
  FuncDef(FuncId id);

  // General metadata.
  FuncId id;
  std::string usr;
  std::string shortName;
  std::string qualifiedName;
  std::optional<clang::SourceLocation> declaration;
  std::optional<clang::SourceLocation> definition;

  // Type which declares this one (ie, it is a method)
  std::optional<TypeId> declaringType;
  // Method this method overrides.
  std::optional<FuncId> baseFunc;
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
};

FuncDef::FuncDef(FuncId id) : id(id) {}

struct VarDef {
  VarDef(VarId id);

  // General metadata.
  VarId id;
  std::string usr;
  std::string shortName;
  std::string qualifiedName;
  std::optional<clang::SourceLocation> declaration;
  std::vector<clang::SourceLocation> initializations;

  // Type of the variable.
  std::optional<TypeId> variableType;

  // Type which declares this one (ie, it is a method)
  std::optional<TypeId> declaringType;

  // Usages.
  std::vector<clang::SourceLocation> uses;
};

VarDef::VarDef(VarId id) : id(id) {}


struct ParsingDatabase {
  // NOTE: Every Id is resolved to a file_id of 0. The correct file_id needs
  //       to get fixed up when inserting into the real db.
  std::unordered_map<std::string, TypeId> usrToTypeId;
  std::unordered_map<std::string, FuncId> usrToFuncId;
  std::unordered_map<std::string, VarId> usrToVarId;

  std::vector<TypeDef> types;
  std::vector<FuncDef> funcs;
  std::vector<VarDef> vars;

  TypeId ToTypeId(const std::string& usr);
  FuncId ToFuncId(const std::string& usr);
  VarId ToVarId(const std::string& usr);

  TypeDef* Resolve(TypeId id);
  FuncDef* Resolve(FuncId id);
  VarDef* Resolve(VarId id);

  std::vector<std::string> ToString();
};

TypeId ParsingDatabase::ToTypeId(const std::string& usr) {
  auto it = usrToTypeId.find(usr);
  if (it != usrToTypeId.end())
    return it->second;

  TypeId id(0, types.size());
  types.push_back(TypeDef(id));
  usrToTypeId[usr] = id;
  return id;
}
FuncId ParsingDatabase::ToFuncId(const std::string& usr) {
  auto it = usrToFuncId.find(usr);
  if (it != usrToFuncId.end())
    return it->second;

  FuncId id(0, funcs.size());
  funcs.push_back(FuncDef(id));
  usrToFuncId[usr] = id;
  return id;
}
VarId ParsingDatabase::ToVarId(const std::string& usr) {
  auto it = usrToVarId.find(usr);
  if (it != usrToVarId.end())
    return it->second;

  VarId id(0, vars.size());
  vars.push_back(VarDef(id));
  usrToVarId[usr] = id;
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

std::vector<std::string> ParsingDatabase::ToString() {
  std::vector<std::string> result;

  result.push_back("Types:");
  for (TypeDef& def : types) {
    result.push_back("  " + def.qualifiedName);
  }

  result.push_back("Funcs:");
  for (FuncDef& def : funcs) {
    result.push_back("  " + def.qualifiedName);
  }

  result.push_back("Vars:");
  for (VarDef& def : vars) {
    result.push_back("  " + def.qualifiedName);
  }

  return result;
}

struct FileDef {
  uint64_t id;
  std::string path;
  std::vector<TypeDef> types;
  std::vector<FuncDef> funcs;
  std::vector<VarDef> vars;
};



struct Database {
  std::unordered_map<std::string, TypeId> usrToTypeId;
  std::unordered_map<std::string, FuncId> usrToFuncId;
  std::unordered_map<std::string, VarId> usrToVarId;

  std::vector<FileDef> files;

  TypeId ToTypeId(const std::string& usr);
  FuncId ToFuncId(const std::string& usr);
  VarId ToVarId(const std::string& usr);
};

TypeId Database::ToTypeId(const std::string& usr) {
  auto it = usrToTypeId.find(usr);
  assert(it != usrToTypeId.end() && "Usr is not registered");
  return it->second;
}
FuncId Database::ToFuncId(const std::string& usr) {
  auto it = usrToFuncId.find(usr);
  assert(it != usrToFuncId.end() && "Usr is not registered");
  return it->second;
}
VarId Database::ToVarId(const std::string& usr) {
  auto it = usrToVarId.find(usr);
  assert(it != usrToVarId.end() && "Usr is not registered");
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


struct NamespaceStack {
  std::vector<std::string> stack;

  void Push(const std::string& ns);
  void Pop();
  std::string ComputeQualifiedPrefix();
};

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




struct FuncDefinitionParam {};

clang::VisiterResult VisitFuncDefinition(clang::Cursor cursor, clang::Cursor parent, FuncDefinitionParam* param) {
  /*
  switch (cursor.get_kind()) {

  default:
    std::cerr << "Unhandled VisitFuncDefinition kind " << clang::ToString(cursor.get_kind()) << std::endl;
    break;
  }
  */

  return clang::VisiterResult::Break;
}

void HandleFunc(ParsingDatabase* db, NamespaceStack* ns, clang::Cursor func, std::optional<TypeId> declaringType) {
  // What this method must process:
  // - function declaration
  // - function definition
  // - method declaration
  // - method inline definition
  // - method definition

  // TODO: Make sure we only process once for declared/defined types.

  // TODO: method_definition_in_namespace.cc is failing because we process decl with correct declaringType, but
  //       processing method definition fails to resolve correct declaringType.
  //if (func.is_definition()) // RM after addressed above
  //  return;                 // RM after addressed above

  FuncId id = db->ToFuncId(func.get_usr());
  db->Resolve(id)->shortName = func.get_spelling();

  std::string typeName;
  if (declaringType)
    typeName = db->Resolve(declaringType.value())->shortName + "::";
  db->Resolve(id)->qualifiedName = ns->ComputeQualifiedPrefix() + typeName + func.get_spelling();
  std::cout << func.get_usr() << ": Set qualified name to " << db->Resolve(id)->qualifiedName << std::endl;

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

  default:
    std::cerr << "Unhandled VisitClassDecl kind " << clang::ToString(cursor.get_kind()) << std::endl;
    break;
  }

  return clang::VisiterResult::Continue;
}

void HandleClassDecl(clang::Cursor cursor, ParsingDatabase* db, NamespaceStack* ns) {
  TypeId active_type = db->ToTypeId(cursor.get_usr());
  db->Resolve(active_type)->shortName = cursor.get_spelling();
  db->Resolve(active_type)->qualifiedName = ns->ComputeQualifiedPrefix() + cursor.get_spelling();

  ClassDeclParam classDeclParam(db, ns, active_type);
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
    if (path != "tests/method_definition_in_namespace.cc") continue;

    std::vector<std::string> expected_output;
    ParseTestExpectation(path, &expected_output);

    std::cout << "[START] " << path << std::endl;

    ParsingDatabase db = Parse(path);
    std::vector<std::string> actual_output = db.ToString();

    if (AreEqual(expected_output, actual_output)) {
      std::cout << "[PASSED] " << path << std::endl;
    }
    else {
      std::cout << "[FAILED] " << path << std::endl;
      std::cout << "Expected output for " << path << ":" << std::endl;
      Write(expected_output);
      std::cout << "Actual output for " << path << ":" << std::endl;
      Write(actual_output);
      break;
    }
  }

  std::cin.get();
  return 0;
}