#include "type_printer.h"

namespace {

int GetNameInsertingPosition(const std::string& type_desc,
                             const std::string& return_type) {
  // Check if type_desc contains an (.
  if (type_desc.size() <= return_type.size() ||
      type_desc.find("(", 0) == std::string::npos)
    return type_desc.size();
  // Find the first character where the return_type differs from the
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
    while (type_desc[ret] == ' ')
      ret++;
  }
  return ret;
}
}  // namespace

// Build a detailed function signature, including argument names.
std::tuple<std::string, int16_t, int16_t, int16_t> GetFunctionSignature(
    IndexFile* db,
    NamespaceHelper* ns,
    const CXIdxDeclInfo* decl) {
  int num_args = clang_Cursor_getNumArguments(decl->cursor);
  int16_t qual_name_offset, short_name_offset, short_name_size;
  std::string func_name;
  std::tie(func_name, short_name_offset, short_name_size) =
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

  std::string type_desc = ClangCursor(decl->cursor).get_type_description();
  qual_name_offset = GetNameInsertingPosition(
      type_desc, ::ToString(clang_getTypeSpelling(
                     clang_getCursorResultType(decl->cursor))));

  if (type_desc[qual_name_offset] == '(') {
    // Find positions to insert argument names.
    // Argument name are before ',' or closing ')'.
    num_args = 0;
    for (int balance = 0, i = qual_name_offset;
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

    // Second pass: insert argument names before each comma and closing paren.
    int i = qual_name_offset;
    short_name_offset += qual_name_offset;
    std::string type_desc_with_names(type_desc.begin(), type_desc.begin() + i);
    type_desc_with_names.append(func_name);
    for (auto& arg : args) {
      if (arg.first < 0)
        break;
      if (arg.second.empty())
        continue;
      // TODO Use inside-out syntax. Note, clang/lib/AST/TypePrinter.cpp does
      // not print arg names.
      type_desc_with_names.insert(type_desc_with_names.end(), &type_desc[i],
                                  &type_desc[arg.first]);
      i = arg.first;
      ConcatTypeAndName(type_desc_with_names, arg.second);
    }
    type_desc_with_names.insert(type_desc_with_names.end(),
                                type_desc.begin() + i, type_desc.end());
    type_desc = std::move(type_desc_with_names);
  } else {
    // type_desc is either a typedef, or some complicated type we cannot handle.
    // Append the func_name in this case.
    int offset = type_desc.size();
    offset += ConcatTypeAndName(type_desc, func_name);
    qual_name_offset = offset;
    short_name_offset += offset;
  }

  return {type_desc, qual_name_offset, short_name_offset, short_name_size};
}
