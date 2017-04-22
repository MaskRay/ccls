#ifndef TRANSLATIONUNIT_H_
#define TRANSLATIONUNIT_H_
#include <clang-c/Index.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Index.h"
#include "Cursor.h"
#include "../language_server_api.h"

namespace clang {
class TranslationUnit {
 public:
  TranslationUnit(IndexerConfig* config,
                  Index& index,
                  const std::string& filepath,
                  const std::vector<std::string>& arguments,
                  std::vector<CXUnsavedFile> unsaved_files,
                  unsigned flags);
  ~TranslationUnit();

  bool did_fail = false;

  void ReparseTranslationUnit(std::vector<CXUnsavedFile>& unsaved);

  Cursor document_cursor() const;

  CXTranslationUnit cx_tu;
};
}  // namespace clang
#endif  // TRANSLATIONUNIT_H_
