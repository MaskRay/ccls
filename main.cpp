#include <iostream>

#include "libclangmm\clangmm.h"
#include "libclangmm\Utility.h"
//#include <clang-c\Index.h>

CXChildVisitResult visitPrint(CXCursor cursor0, CXCursor parent, CXClientData param) {
  int* level = static_cast<int*>(param);

  clang::Cursor cursor(cursor0);

  for (int i = 0; i < *level; ++i)
    std::cout << "  ";
  std::cout << cursor.get_spelling() << " " << clang::to_string(cursor.get_kind()) << std::endl;

  *level += 1;
  clang_visitChildren(cursor0, &visitPrint, level);
  *level -= 1;

  return CXChildVisitResult::CXChildVisit_Continue;
/*
  echo "  ".repeat(param.level), cursor.kind, " ", getSpelling(cursor)

  var visitParam : VisitPrintParam
  visitParam.level = param.level + 1
  visitChildren(cursor, visitPrint, addr visitParam)

  return VisitResult.Continu
*/
}

int main(int argc, char** argv) {
  /*
    echo "Parsing ", filename
    let index : libclang.CXIndex = libclang.createIndex(#[excludeDeclsFromPCH]# 0, #[displayDiagnostics]# 0)

    let translationUnit : libclang.CXTranslationUnit =
    libclang.parseTranslationUnit(index, filename.cstring, nil, 0, nil, 0, 0)
    if translationUnit == nil :
      echo "Error: cannot create translation unit for '", filename, "'"
      return

    let cursor = libclang.getTranslationUnitCursor(translationUnit)

    var param : VisitFileParam
    param.db = db

    echo "=== START AST ==="
    # printChildren(cursor, 0)
    echo "=== DONE AST ==="

    var file : File
    file.path = filename
    file.vars = newList[VarRef]()
    file.funcs = newList[FuncRef]()
    file.types = newList[TypeRef]()

    visitChildren(cursor, visitFile, addr param)

    libclang.disposeTranslationUnit(translationUnit)
    libclang.disposeIndex(index)
  */

  std::vector<std::string> args;

  clang::Index index(0 /*excludeDeclarationsFromPCH*/, 0 /*displayDiagnostics*/);
  clang::TranslationUnit translationUnit(index, "test.cc", args);

  int level = 0;
  auto cursor = clang_getTranslationUnitCursor(translationUnit.cx_tu);
  clang_visitChildren(cursor, &visitPrint, &level);
  /*
  TranslationUnit(Index &index,
    const std::string &file_path,
    const std::vector<std::string> &command_line_args,
    const std::string &buffer,
    unsigned flags = DefaultFlags());
  TranslationUnit(Index &index,
    const std::string &file_path,
    const std::vector<std::string> &command_line_args,
    unsigned flags = DefaultFlags());
  */

  std::cin.get();
}