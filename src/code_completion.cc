#include <clang-c/Index.h>

void doit() {
  // we should probably main two translation units, one for
  // serving current requests, and one that is reparsing (follow qtcreator)

  // use clang_codeCompleteAt

  // we need to setup CXUnsavedFile
  // The key to doing that is via
  //  - textDocument/didOpen
  //  - textDocument/didChange
  //  - textDocument/didClose

  // probably don't need
  //  - textDocument/willSave
}