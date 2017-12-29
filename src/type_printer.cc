#include "type_printer.h"
#include <string>
#include "loguru.hpp"

#if USE_CLANG_CXX
# include "CXTranslationUnit.h"
# include "clang/AST/Type.h"
# include "clang/AST/PrettyPrinter.h"
# include "clang/Frontend/ASTUnit.h"
# include "llvm/ADT/SmallString.h"
# include "llvm/Support/raw_ostream.h"

using namespace clang;

// Extracted from clang/tools/libclang/CXType.cpp
static inline QualType GetQualType(CXType CT) {
  return QualType::getFromOpaquePtr(CT.data[0]);
}

static inline CXTranslationUnit GetTU(CXType CT) {
  return static_cast<CXTranslationUnit>(CT.data[1]);
}
#endif

namespace {

int GetNameInsertingPosition(const std::string& type_desc,
                             const std::string& return_type) {
  // Check if type_desc contains an (.
  if (type_desc.empty() || type_desc.size() <= return_type.size() ||
      type_desc.find("(", 0) == std::string::npos)
    return -1;
  // Find a first character where the return_type differs from the
  // function_type. In most cases this is the place where the function name
  // should be inserted.
  int ret = 0;
  while (return_type[ret] == type_desc[ret])
    ret++;

  if (ret == 0) {
    // If return type and function type do not overlap at all,
    // check if it is `auto (...) ->` trailing return type.
    if (type_desc.compare(0, 5, "auto ") == 0 &&
        type_desc.find(" -> ") != std::string::npos)
      ret = 5;
  } else {
    // Otherwise return type is just a prefix of the function_type.
    // Skip any eventual spaces after the return type.
    while (ret < int(type_desc.size()) && type_desc[ret] == ' ')
      ret++;
  }
  return ret;
}
}  // namespace

// Build a detailed function signature, including argument names.
std::string GetFunctionSignature(IndexFile* db,
                                 NamespaceHelper* ns,
                                 const CXIdxDeclInfo* decl) {
  int num_args = clang_Cursor_getNumArguments(decl->cursor);
  std::string function_name =
      ns->QualifiedName(decl->semanticContainer, decl->entityInfo->name);

  std::vector<std::pair<int, std::string>> args;
  for (int i = 0; i < num_args; i++) {
    args.emplace_back(-1, ::ToString(clang_getCursorDisplayName(
                              clang_Cursor_getArgument(decl->cursor, i))));
  }
  if (clang_Cursor_isVariadic(decl->cursor)) {
    args.emplace_back(-1, "");
    num_args++;
  }

  std::string type_desc;
  int function_name_offset;
#if USE_CLANG_CXX
  {
    CXType CT = clang_getCursorType(decl->cursor);
    QualType T = GetQualType(CT);
    if (!T.isNull()) {
      CXTranslationUnit TU = GetTU(CT);
      SmallString<64> Str;
      llvm::raw_svector_ostream OS(Str);
      PrintingPolicy PP(cxtu::getASTUnit(TU)->getASTContext().getLangOpts());

      T.print(OS, PP, "=^_^=");
      type_desc = OS.str();
      function_name_offset = type_desc.find("=^_^=");
      if (type_desc[function_name_offset + 5] != ')')
        type_desc = type_desc.replace(function_name_offset, 5, "");
      else {
        type_desc = type_desc.replace(function_name_offset, 6, "");
        for (int i = function_name_offset; i-- > 0; )
          if (type_desc[i] == '(') {
            type_desc.erase(type_desc.begin() + i);
            break;
          }
        function_name_offset--;
      }
    }
  }
#else
  type_desc = ClangCursor(decl->cursor).get_type_description();
  std::string return_type = ::ToString(
      clang_getTypeSpelling(clang_getCursorResultType(decl->cursor)));
  function_name_offset = GetNameInsertingPosition(type_desc, return_type);
#endif

  if (function_name_offset >= 0 && type_desc[function_name_offset] == '(') {
    if (num_args > 0) {
      // Find positions to insert argument names.
      // Last argument name is before ')'
      num_args = 0;
      // Other argument names come before ','
      for (int balance = 0, i = function_name_offset;
           i < int(type_desc.size()) && num_args < int(args.size()); i++) {
        if (type_desc[i] == '(' || type_desc[i] == '[')
          balance++;
        else if (type_desc[i] == ')' || type_desc[i] == ']') {
          if (--balance <= 0) {
            args[num_args].first = i;
            break;
          }
        } else if (type_desc[i] == ',' && balance == 1)
          args[num_args++].first = i;
      }

      // Second pass: Insert argument names before each comma.
      int i = 0;
      std::string type_desc_with_names;
      for (auto& arg : args) {
        if (arg.first < 0) {
          LOG_S(ERROR)
              << "When adding argument names to '" << type_desc
              << "', failed to detect positions to insert argument names";
          break;
        }
        if (arg.second.empty())
          continue;
        // TODO Use inside-out syntax. Note, clang/lib/AST/TypePrinter.cpp does
        // not print arg names.
        type_desc_with_names.insert(type_desc_with_names.end(), &type_desc[i],
                                    &type_desc[arg.first]);
        i = arg.first;
        if (type_desc_with_names.size() &&
            (type_desc_with_names.back() != ' ' &&
             type_desc_with_names.back() != '*' &&
             type_desc_with_names.back() != '&'))
          type_desc_with_names.push_back(' ');
        type_desc_with_names.append(arg.second);
      }
      type_desc_with_names.insert(type_desc_with_names.end(),
                                  type_desc.begin() + i, type_desc.end());
      type_desc = std::move(type_desc_with_names);
    }

    type_desc.insert(function_name_offset, function_name);
  } else {
    // type_desc is either a typedef, or some complicated type we cannot handle.
    // Append the function_name in this case.
    if (type_desc.size() &&
        (type_desc.back() != ' ' &&
         type_desc.back() != '*' &&
         type_desc.back() != '&'))
      type_desc.push_back(' ');
    type_desc.append(function_name);
  }

  return type_desc;
}
