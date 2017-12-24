#include "indexer.h"

#include "clang_cursor.h"
#include "clang_utils.h"
#include "platform.h"
#include "serializer.h"
#include "timer.h"

#include <loguru.hpp>

#include <algorithm>
#include <chrono>
#include <climits>

// TODO: See if we can use clang_indexLoc_getFileLocation to get a type ref on
// |Foobar| in DISALLOW_COPY(Foobar)

namespace {

constexpr bool kIndexStdDeclarations = true;

// For typedef/using spanning less than or equal to (this number) of lines,
// display their declarations on hover.
constexpr int kMaxLinesDisplayTypeAliasDeclarations = 3;

void AddFuncRef(std::vector<IndexFuncRef>* result, IndexFuncRef ref) {
  if (!result->empty() && (*result)[result->size() - 1] == ref)
    return;
  result->push_back(ref);
}

Range Resolve(const CXSourceRange& range, CXFile* cx_file = nullptr) {
  CXSourceLocation start = clang_getRangeStart(range);
  CXSourceLocation end = clang_getRangeEnd(range);

  unsigned int start_line, start_column;
  clang_getSpellingLocation(start, cx_file, &start_line, &start_column,
                            nullptr);
  unsigned int end_line, end_column;
  clang_getSpellingLocation(end, nullptr, &end_line, &end_column, nullptr);

  return Range(Position((int16_t)start_line, (int16_t)start_column) /*start*/,
               Position((int16_t)end_line, (int16_t)end_column) /*end*/);
}

Range ResolveSpelling(const CXCursor& cx_cursor, CXFile* cx_file = nullptr) {
  CXSourceRange cx_range = clang_Cursor_getSpellingNameRange(cx_cursor, 0, 0);
  return Resolve(cx_range, cx_file);
}

Range ResolveExtent(const CXCursor& cx_cursor, CXFile* cx_file = nullptr) {
  CXSourceRange cx_range = clang_getCursorExtent(cx_cursor);
  return Resolve(cx_range, cx_file);
}

bool IsScopeSemanticContainer(CXCursorKind kind) {
  switch (kind) {
    case CXCursor_Namespace:
    case CXCursor_TranslationUnit:
    case CXCursor_StructDecl:
    case CXCursor_UnionDecl:
    case CXCursor_ClassDecl:
    case CXCursor_EnumDecl:
    // TODO Add more Objective-C containers
    case CXCursor_ObjCInterfaceDecl:
    case CXCursor_ObjCImplementationDecl:
      return false;
    default:
      return true;
  }
}

struct NamespaceHelper {
  std::unordered_map<ClangCursor, std::string>
      container_cursor_to_qualified_name;

  void RegisterQualifiedName(std::string usr,
                             const CXIdxContainerInfo* container,
                             std::string qualified_name) {}

  std::string QualifiedName(const CXIdxContainerInfo* container,
                            std::string unqualified_name) {
    if (!container)
      return unqualified_name;
    // Anonymous namespaces are not processed by indexDeclaration. We trace
    // nested namespaces bottom-up through clang_getCursorSemanticParent until
    // one that we know its qualified name. Then do another trace top-down and
    // put their names into a map of USR -> qualified_name.
    ClangCursor cursor = container->cursor;
    std::vector<ClangCursor> namespaces;
    std::string qualifier;
    while (cursor.get_kind() != CXCursor_TranslationUnit &&
           !IsScopeSemanticContainer(cursor.get_kind())) {
      auto it = container_cursor_to_qualified_name.find(cursor);
      if (it != container_cursor_to_qualified_name.end()) {
        qualifier = it->second;
        break;
      }
      namespaces.push_back(cursor);
      cursor = clang_getCursorSemanticParent(cursor.cx_cursor);
    }
    for (size_t i = namespaces.size(); i > 0;) {
      i--;
      std::string name = namespaces[i].get_spelling();
      // Empty name indicates unnamed namespace, anonymous struct, anonymous
      // union, ...
      qualifier += name.empty() ? "(anon)" : name;
      qualifier += "::";
      container_cursor_to_qualified_name[namespaces[i]] = qualifier;
    }
    return qualifier + unqualified_name;
  }
};

// Caches all instances of constructors, regardless if they are indexed or not.
// The constructor may have a make_unique call associated with it that we need
// to export. If we do not capture the parameter type description for the
// constructor we will not be able to attribute the constructor call correctly.
struct ConstructorCache {
  using Usr = std::string;
  struct Constructor {
    Usr usr;
    std::vector<std::string> param_type_desc;
  };
  std::unordered_map<Usr, std::vector<Constructor>> constructors_;

  // This should be called whenever there is a constructor declaration.
  void NotifyConstructor(ClangCursor ctor_cursor) {
    auto build_type_desc = [](ClangCursor cursor) {
      std::vector<std::string> type_desc;
      for (ClangCursor arg : cursor.get_arguments()) {
        if (arg.get_kind() == CXCursor_ParmDecl)
          type_desc.push_back(arg.get_type_description());
      }
      return type_desc;
    };

    Constructor ctor{ctor_cursor.get_usr(), build_type_desc(ctor_cursor)};

    // Insert into |constructors_|.
    std::string type_usr = ctor_cursor.get_semantic_parent().get_usr();
    auto existing_ctors = constructors_.find(type_usr);
    if (existing_ctors != constructors_.end()) {
      existing_ctors->second.push_back(ctor);
    } else {
      constructors_[type_usr] = {ctor};
    }
  }

  // Tries to lookup a constructor in |type_usr| that takes arguments most
  // closely aligned to |param_type_desc|.
  optional<std::string> TryFindConstructorUsr(
      const std::string& type_usr,
      const std::vector<std::string>& param_type_desc) {
    auto count_matching_prefix_length = [](const char* a, const char* b) {
      int matched = 0;
      while (*a && *b) {
        if (*a != *b)
          break;
        ++a;
        ++b;
        ++matched;
      }
      // Additional score if the strings were the same length, which makes
      // "a"/"a" match higher than "a"/"a&"
      if (*a == *b)
        matched += 1;
      return matched;
    };

    // Try to find constructors for the type. If there are no constructors
    // available, return an empty result.
    auto ctors_it = constructors_.find(type_usr);
    if (ctors_it == constructors_.end())
      return nullopt;
    const std::vector<Constructor>& ctors = ctors_it->second;
    if (ctors.empty())
      return nullopt;

    std::string best_usr;
    int best_score = INT_MIN;

    // Scan constructors for the best possible match.
    for (const Constructor& ctor : ctors) {
      // If |param_type_desc| is empty and the constructor is as well, we don't
      // need to bother searching, as this is the match.
      if (param_type_desc.empty() && ctor.param_type_desc.empty()) {
        best_usr = ctor.usr;
        break;
      }

      // Weight matching parameter length heavily, as it is more accurate than
      // the fuzzy type matching approach.
      int score = 0;
      if (param_type_desc.size() == ctor.param_type_desc.size())
        score += param_type_desc.size() * 1000;

      // Do prefix-based match on parameter type description. This works well in
      // practice because clang appends qualifiers to the end of the type, ie,
      // |foo *&&|
      for (int i = 0;
           i < std::min(param_type_desc.size(), ctor.param_type_desc.size());
           ++i) {
        score += count_matching_prefix_length(param_type_desc[i].c_str(),
                                              ctor.param_type_desc[i].c_str());
      }

      if (score > best_score) {
        best_usr = ctor.usr;
        best_score = score;
      }
    }

    assert(!best_usr.empty());
    return best_usr;
  }
};

struct IndexParam {
  std::unordered_set<CXFile> seen_cx_files;
  std::vector<std::string> seen_files;
  std::unordered_map<std::string, FileContentsWithOffsets> file_contents;
  std::unordered_map<std::string, int64_t> file_modification_times;

  // Only use this when strictly needed (ie, primary translation unit is
  // needed). Most logic should get the IndexFile instance via
  // |file_consumer|.
  //
  // This can be null if we're not generating an index for the primary
  // translation unit.
  IndexFile* primary_file = nullptr;

  ClangTranslationUnit* tu = nullptr;

  FileConsumer* file_consumer = nullptr;
  NamespaceHelper ns;
  ConstructorCache ctors;

  IndexParam(ClangTranslationUnit* tu, FileConsumer* file_consumer)
      : tu(tu), file_consumer(file_consumer) {}
};

IndexFile* ConsumeFile(IndexParam* param, CXFile file) {
  bool is_first_ownership = false;
  IndexFile* db =
      param->file_consumer->TryConsumeFile(file, &is_first_ownership);

  // If this is the first time we have seen the file (ignoring if we are
  // generating an index for it):
  if (param->seen_cx_files.insert(file).second) {
    std::string file_name = FileName(file);
    // Sometimes the fill name will be empty. Not sure why. Not much we can do
    // with it.
    if (!file_name.empty()) {
      // Add to all files we have seen so we can generate proper dependency
      // graph.
      param->seen_files.push_back(file_name);

      // Set modification time.
      optional<int64_t> modification_time = GetLastModificationTime(file_name);
      LOG_IF_S(ERROR, !modification_time)
          << "Failed fetching modification time for " << file_name;
      if (modification_time)
        param->file_modification_times[file_name] = *modification_time;

      // Capture file contents in |param->file_contents| if it was not specified
      // at the start of indexing.
      if (db && !param->file_contents.count(file_name)) {
        optional<std::string> content = ReadContent(file_name);
        if (content)
          param->file_contents.emplace(file_name, *content);
        else
          LOG_S(ERROR) << "[indexer] Failed to read file content for "
                       << file_name;
      }
    }
  }

  if (is_first_ownership) {
    // Report skipped source range list.
    CXSourceRangeList* skipped = clang_getSkippedRanges(param->tu->cx_tu, file);
    for (unsigned i = 0; i < skipped->count; ++i) {
      Range range = Resolve(skipped->ranges[i]);
      // clang_getSkippedRanges reports start one token after the '#', move it
      // back so it starts at the '#'
      range.start.column -= 1;
      db->skipped_by_preprocessor.push_back(range);
    }
    clang_disposeSourceRangeList(skipped);
  }

  return db;
}

// Returns true if the given entity kind can be called implicitly, ie, without
// actually being written in the source code.
bool CanBeCalledImplicitly(CXIdxEntityKind kind) {
  switch (kind) {
    case CXIdxEntity_CXXConstructor:
    case CXIdxEntity_CXXConversionFunction:
    case CXIdxEntity_CXXDestructor:
      return true;
    default:
      return false;
  }
}

// Returns true if the cursor spelling contains the given string. This is
// useful to check for implicit function calls.
bool CursorSpellingContainsString(CXCursor cursor,
                                  CXTranslationUnit cx_tu,
                                  std::string scanning_for) {
  CXSourceRange range = clang_Cursor_getSpellingNameRange(cursor, 0, 0);
  CXToken* tokens;
  unsigned num_tokens;
  clang_tokenize(cx_tu, range, &tokens, &num_tokens);

  bool result = false;

  for (unsigned i = 0; i < num_tokens; ++i) {
    CXString name = clang_getTokenSpelling(cx_tu, tokens[i]);
    if (strcmp(clang_getCString(name), scanning_for.c_str()) == 0) {
      result = true;
      break;
    }
    clang_disposeString(name);
  }

  clang_disposeTokens(cx_tu, tokens, num_tokens);
  return result;
}

// Returns the document content for the given range. May not work perfectly
// when there are tabs instead of spaces.
std::string GetDocumentContentInRange(CXTranslationUnit cx_tu,
                                      CXSourceRange range) {
  std::string result;

  CXToken* tokens;
  unsigned num_tokens;
  clang_tokenize(cx_tu, range, &tokens, &num_tokens);

  optional<Range> previous_token_range;

  for (unsigned i = 0; i < num_tokens; ++i) {
    // Add whitespace between the previous token and this one.
    Range token_range = Resolve(clang_getTokenExtent(cx_tu, tokens[i]));
    if (previous_token_range) {
      // Insert newlines.
      int16_t line_delta =
          token_range.start.line - previous_token_range->end.line;
      assert(line_delta >= 0);
      if (line_delta > 0) {
        result.append((size_t)line_delta, '\n');
        // Reset column so we insert starting padding.
        previous_token_range->end.column = 0;
      }
      // Insert spaces.
      int16_t column_delta =
          token_range.start.column - previous_token_range->end.column;
      assert(column_delta >= 0);
      result.append((size_t)column_delta, ' ');
    }
    previous_token_range = token_range;

    // Add token content.
    CXString spelling = clang_getTokenSpelling(cx_tu, tokens[i]);
    result += clang_getCString(spelling);
    clang_disposeString(spelling);
  }

  clang_disposeTokens(cx_tu, tokens, num_tokens);

  return result;
}

}  // namespace

// static
int IndexFile::kCurrentVersion = 6;

IndexFile::IndexFile(const std::string& path) : id_cache(path), path(path) {
  // TODO: Reconsider if we should still be reusing the same id_cache.
  // Preallocate any existing resolved ids.
  for (const auto& entry : id_cache.usr_to_type_id)
    types.push_back(IndexType(entry.second, entry.first));
  for (const auto& entry : id_cache.usr_to_func_id)
    funcs.push_back(IndexFunc(entry.second, entry.first));
  for (const auto& entry : id_cache.usr_to_var_id)
    vars.push_back(IndexVar(entry.second, entry.first));
}

// TODO: Optimize for const char*?
IndexTypeId IndexFile::ToTypeId(const std::string& usr) {
  auto it = id_cache.usr_to_type_id.find(usr);
  if (it != id_cache.usr_to_type_id.end())
    return it->second;

  IndexTypeId id(types.size());
  types.push_back(IndexType(id, usr));
  id_cache.usr_to_type_id[usr] = id;
  id_cache.type_id_to_usr[id] = usr;
  return id;
}
IndexFuncId IndexFile::ToFuncId(const std::string& usr) {
  auto it = id_cache.usr_to_func_id.find(usr);
  if (it != id_cache.usr_to_func_id.end())
    return it->second;

  IndexFuncId id(funcs.size());
  funcs.push_back(IndexFunc(id, usr));
  id_cache.usr_to_func_id[usr] = id;
  id_cache.func_id_to_usr[id] = usr;
  return id;
}
IndexVarId IndexFile::ToVarId(const std::string& usr) {
  auto it = id_cache.usr_to_var_id.find(usr);
  if (it != id_cache.usr_to_var_id.end())
    return it->second;

  IndexVarId id(vars.size());
  vars.push_back(IndexVar(id, usr));
  id_cache.usr_to_var_id[usr] = id;
  id_cache.var_id_to_usr[id] = usr;
  return id;
}

IndexTypeId IndexFile::ToTypeId(const CXCursor& cursor) {
  return ToTypeId(ClangCursor(cursor).get_usr());
}

IndexFuncId IndexFile::ToFuncId(const CXCursor& cursor) {
  return ToFuncId(ClangCursor(cursor).get_usr());
}

IndexVarId IndexFile::ToVarId(const CXCursor& cursor) {
  return ToVarId(ClangCursor(cursor).get_usr());
}

IndexType* IndexFile::Resolve(IndexTypeId id) {
  return &types[id.id];
}
IndexFunc* IndexFile::Resolve(IndexFuncId id) {
  return &funcs[id.id];
}
IndexVar* IndexFile::Resolve(IndexVarId id) {
  return &vars[id.id];
}

std::string IndexFile::ToString() {
  return Serialize(*this);
}

IndexType::IndexType(IndexTypeId id, const std::string& usr)
    : usr(usr), id(id) {
  assert(usr.size() > 0);
}

void RemoveItem(std::vector<Range>& ranges, Range to_remove) {
  auto it = std::find(ranges.begin(), ranges.end(), to_remove);
  if (it != ranges.end())
    ranges.erase(it);
}

template <typename T>
void UniqueAdd(std::vector<T>& values, T value) {
  if (std::find(values.begin(), values.end(), value) == values.end())
    values.push_back(value);
}

IdCache::IdCache(const std::string& primary_file)
    : primary_file(primary_file) {}

template <typename T>
bool Contains(const std::vector<T>& vec, const T& element) {
  for (const T& entry : vec) {
    if (entry == element)
      return true;
  }
  return false;
}

void OnIndexDiagnostic(CXClientData client_data,
                       CXDiagnosticSet diagnostics,
                       void* reserved) {
  IndexParam* param = static_cast<IndexParam*>(client_data);

  for (unsigned i = 0; i < clang_getNumDiagnosticsInSet(diagnostics); ++i) {
    CXDiagnostic diagnostic = clang_getDiagnosticInSet(diagnostics, i);

    // Skip diagnostics in system headers.
    CXSourceLocation diag_loc = clang_getDiagnosticLocation(diagnostic);
    if (clang_Location_isInSystemHeader(diag_loc))
      continue;

    // Get db so we can attribute diagnostic to the right indexed file.
    CXFile file;
    unsigned int line, column;
    clang_getSpellingLocation(diag_loc, &file, &line, &column, nullptr);
    IndexFile* db = ConsumeFile(param, file);
    if (!db)
      continue;

    // Build diagnostic.
    optional<lsDiagnostic> ls_diagnostic =
        BuildAndDisposeDiagnostic(diagnostic, db->path);
    if (ls_diagnostic)
      db->diagnostics_.push_back(*ls_diagnostic);
  }
}

CXIdxClientFile OnIndexIncludedFile(CXClientData client_data,
                                    const CXIdxIncludedFileInfo* file) {
  IndexParam* param = static_cast<IndexParam*>(client_data);

  // file->hashLoc only has the position of the hash. We don't have the full
  // range for the include.
  CXSourceLocation hash_loc = clang_indexLoc_getCXSourceLocation(file->hashLoc);
  CXFile cx_file;
  unsigned int line;
  clang_getSpellingLocation(hash_loc, &cx_file, &line, nullptr, nullptr);

  IndexFile* db = ConsumeFile(param, cx_file);
  if (!db)
    return nullptr;

  IndexInclude include;
  include.line = line;
  include.resolved_path = FileName(file->file);
  if (!include.resolved_path.empty())
    db->includes.push_back(include);

  return nullptr;
}

ClangCursor::VisitResult DumpVisitor(ClangCursor cursor,
                                     ClangCursor parent,
                                     int* level) {
  for (int i = 0; i < *level; ++i)
    std::cerr << "  ";
  std::cerr << ToString(cursor.get_kind()) << " " << cursor.get_spelling()
            << std::endl;

  *level += 1;
  cursor.VisitChildren(&DumpVisitor, level);
  *level -= 1;

  return ClangCursor::VisitResult::Continue;
}

void Dump(ClangCursor cursor) {
  int level = 0;
  cursor.VisitChildren(&DumpVisitor, &level);
}

struct FindChildOfKindParam {
  CXCursorKind target_kind;
  optional<ClangCursor> result;

  FindChildOfKindParam(CXCursorKind target_kind) : target_kind(target_kind) {}
};

ClangCursor::VisitResult FindChildOfKindVisitor(ClangCursor cursor,
                                                ClangCursor parent,
                                                FindChildOfKindParam* param) {
  if (cursor.get_kind() == param->target_kind) {
    param->result = cursor;
    return ClangCursor::VisitResult::Break;
  }

  return ClangCursor::VisitResult::Recurse;
}

optional<ClangCursor> FindChildOfKind(ClangCursor cursor, CXCursorKind kind) {
  FindChildOfKindParam param(kind);
  cursor.VisitChildren(&FindChildOfKindVisitor, &param);
  return param.result;
}

ClangCursor::VisitResult FindTypeVisitor(ClangCursor cursor,
                                         ClangCursor parent,
                                         optional<ClangCursor>* result) {
  switch (cursor.get_kind()) {
    case CXCursor_TypeRef:
    case CXCursor_TemplateRef:
      *result = cursor;
      return ClangCursor::VisitResult::Break;
    default:
      break;
  }

  return ClangCursor::VisitResult::Recurse;
}

optional<ClangCursor> FindType(ClangCursor cursor) {
  optional<ClangCursor> result;
  cursor.VisitChildren(&FindTypeVisitor, &result);
  return result;
}

bool IsGlobalContainer(const CXIdxContainerInfo* container) {
  if (!container)
    return false;

  switch (container->cursor.kind) {
    case CXCursor_Namespace:
    case CXCursor_TranslationUnit:
      return true;
    default:
      return false;
  }
}

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
  IndexFile* db;
  int has_processed_any = false;
  optional<ClangCursor> previous_cursor;
  optional<IndexTypeId> initial_type;

  VisitDeclForTypeUsageParam(IndexFile* db) : db(db) {}
};

void VisitDeclForTypeUsageVisitorHandler(ClangCursor cursor,
                                         VisitDeclForTypeUsageParam* param) {
  param->has_processed_any = true;
  IndexFile* db = param->db;

  std::string referenced_usr =
      cursor.get_referenced()
          .template_specialization_to_template_definition()
          .get_usr();
  // TODO: things in STL cause this to be empty. Figure out why and document it.
  if (referenced_usr == "")
    return;

  IndexTypeId ref_type_id = db->ToTypeId(referenced_usr);

  if (!param->initial_type)
    param->initial_type = ref_type_id;

  IndexType* ref_type_def = db->Resolve(ref_type_id);
  // TODO: Should we even be visiting this if the file is not from the main
  // def? Try adding assert on |loc| later.
  Range loc = ResolveSpelling(cursor.cx_cursor);
  UniqueAdd(ref_type_def->uses, loc);
}

ClangCursor::VisitResult VisitDeclForTypeUsageVisitor(
    ClangCursor cursor,
    ClangCursor parent,
    VisitDeclForTypeUsageParam* param) {
  switch (cursor.get_kind()) {
    case CXCursor_TemplateRef:
    case CXCursor_TypeRef:
      if (param->previous_cursor) {
        VisitDeclForTypeUsageVisitorHandler(param->previous_cursor.value(),
                                            param);
      }

      param->previous_cursor = cursor;
      return ClangCursor::VisitResult::Continue;

    // We do not want to recurse for everything, since if we do that we will end
    // up visiting method definition bodies/etc. Instead, we only recurse for
    // things that can logically appear as part of an inline variable
    // initializer,
    // ie,
    //
    //  class Foo {
    //   int x = (Foo)3;
    //  }
    case CXCursor_CallExpr:
    case CXCursor_CStyleCastExpr:
    case CXCursor_CXXStaticCastExpr:
    case CXCursor_CXXReinterpretCastExpr:
      return ClangCursor::VisitResult::Recurse;

    default:
      return ClangCursor::VisitResult::Continue;
  }

  return ClangCursor::VisitResult::Continue;
}

// Finds the cursor associated with the declaration type of |cursor|. This
// strips
// qualifies from |cursor| (ie, Foo* => Foo) and removes template arguments
// (ie, Foo<A,B> => Foo<*,*>).
optional<IndexTypeId> ResolveToDeclarationType(IndexFile* db,
                                               ClangCursor cursor) {
  ClangCursor declaration = cursor.get_declaration();
  declaration = declaration.template_specialization_to_template_definition();
  std::string usr = declaration.get_usr();
  if (usr != "")
    return db->ToTypeId(usr);
  return nullopt;
}

// Add usages to any seen TypeRef or TemplateRef under the given |decl_cursor|.
// This returns the first seen TypeRef or TemplateRef value, which can be
// useful if trying to figure out ie, what a using statement refers to. If
// trying to generally resolve a cursor to a type, use
// ResolveToDeclarationType, which works in more scenarios.
optional<IndexTypeId> AddDeclTypeUsages(
    IndexFile* db,
    ClangCursor decl_cursor,
    const CXIdxContainerInfo* semantic_container,
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
  //
  // Here is another example:
  //
  //  enum A {};
  //  enum B {};
  //
  //  template<typename T>
  //  struct Foo {
  //    struct Inner {};
  //  };
  //
  //  Foo<A>::Inner a;
  //  Foo<B> b;
  //
  //  =>
  //
  //  EnumDecl A
  //  EnumDecl B
  //  ClassTemplate Foo
  //    TemplateTypeParameter T
  //    StructDecl Inner
  //  VarDecl a
  //    TemplateRef Foo
  //    TypeRef enum A
  //    TypeRef struct Foo<enum A>::Inner
  //    CallExpr Inner
  //  VarDecl b
  //    TemplateRef Foo
  //    TypeRef enum B
  //    CallExpr Foo
  //
  //
  // Determining the actual type of the variable/declaration from just the
  // children is tricky. Doing so would require looking up the template
  // definition associated with a TemplateRef, figuring out how many children
  // it has, and then skipping that many TypeRef values. This also has to work
  // with the example below (skipping the last TypeRef). As a result, we
  // determine variable types using |ResolveToDeclarationType|.
  //
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
  if (IsTypeDefinition(semantic_container) &&
      !IsTypeDefinition(lexical_container)) {
    //
    // In some code, such as the following example, we receive a cursor which is
    // not
    // a definition and is not associated with a definition due to an error
    // condition.
    // In this case, it is the Foo::Foo constructor.
    //
    //  struct Foo {};
    //
    //  template<class T>
    //  Foo::Foo() {}
    //
    if (!decl_cursor.is_definition()) {
      ClangCursor def = decl_cursor.get_definition();
      if (def.get_kind() != CXCursor_FirstInvalid)
        decl_cursor = def;
    }
    process_last_type_ref = false;
  }

  VisitDeclForTypeUsageParam param(db);
  decl_cursor.VisitChildren(&VisitDeclForTypeUsageVisitor, &param);

  // VisitDeclForTypeUsageVisitor guarantees that if there are multiple TypeRef
  // children, the first one will always be visited.
  if (param.previous_cursor && process_last_type_ref) {
    VisitDeclForTypeUsageVisitorHandler(param.previous_cursor.value(), &param);
  } else {
    // If we are not processing the last type ref, it *must* be a TypeRef or
    // TemplateRef.
    //
    // We will not visit every child if the is_interseting is false, so
    // previous_cursor
    // may not point to the last TemplateRef.
    assert(param.previous_cursor.has_value() == false ||
           (param.previous_cursor.value().get_kind() == CXCursor_TypeRef ||
            param.previous_cursor.value().get_kind() == CXCursor_TemplateRef));
  }

  return param.initial_type;
}

// Various versions of LLVM (ie, 4.0) will not visit inline variable references
// for template arguments.
ClangCursor::VisitResult AddDeclInitializerUsagesVisitor(ClangCursor cursor,
                                                         ClangCursor parent,
                                                         IndexFile* db) {
  /*
    We need to index the |DeclRefExpr| below (ie, |var| inside of
    Foo<int>::var).

      template<typename T>
      struct Foo {
        static constexpr int var = 3;
      };

      int a = Foo<int>::var;

      =>

      VarDecl a
        UnexposedExpr var
          DeclRefExpr var
            TemplateRef Foo

  */

  switch (cursor.get_kind()) {
    case CXCursor_DeclRefExpr: {
      if (cursor.get_referenced().get_kind() != CXCursor_VarDecl)
        break;

      // TODO: when we resolve the template type to the definition, we get a
      // different USR.

      // ClangCursor ref =
      // cursor.get_referenced().template_specialization_to_template_definition().get_type().strip_qualifiers().get_usr();
      // std::string ref_usr =
      // cursor.get_referenced().template_specialization_to_template_definition().get_type().strip_qualifiers().get_usr();
      std::string ref_usr =
          cursor.get_referenced()
              .template_specialization_to_template_definition()
              .get_usr();
      // std::string ref_usr = ref.get_usr();
      if (ref_usr == "")
        break;

      Range loc = ResolveSpelling(cursor.cx_cursor);
      IndexVarId ref_id = db->ToVarId(ref_usr);
      IndexVar* ref_def = db->Resolve(ref_id);
      UniqueAdd(ref_def->uses, loc);
      break;
    }

    default:
      break;
  }

  return ClangCursor::VisitResult::Recurse;
}

void AddDeclInitializerUsages(IndexFile* db, ClangCursor decl_cursor) {
  decl_cursor.VisitChildren(&AddDeclInitializerUsagesVisitor, db);
}

bool AreEqualLocations(CXIdxLoc loc, CXCursor cursor) {
  // clang_getCursorExtent
  // clang_Cursor_getSpellingNameRange

  return clang_equalLocations(
      clang_indexLoc_getCXSourceLocation(loc),
      // clang_getRangeStart(clang_getCursorExtent(cursor)));
      clang_getRangeStart(clang_Cursor_getSpellingNameRange(cursor, 0, 0)));
}

ClangCursor::VisitResult VisitMacroDefinitionAndExpansions(ClangCursor cursor,
                                                           ClangCursor parent,
                                                           IndexParam* param) {
  switch (cursor.get_kind()) {
    case CXCursor_MacroDefinition:
    case CXCursor_MacroExpansion: {
      // Resolve location, find IndexFile instance.
      CXSourceRange cx_source_range =
          clang_Cursor_getSpellingNameRange(cursor.cx_cursor, 0, 0);
      CXSourceLocation start = clang_getRangeStart(cx_source_range);
      if (clang_Location_isInSystemHeader(start))
        break;
      CXFile file;
      Range decl_loc_spelling = Resolve(cx_source_range, &file);
      IndexFile* db = ConsumeFile(param, file);
      if (!db)
        break;

      // TODO: Considering checking clang_Cursor_isMacroFunctionLike, but the
      // only real difference will be that we show 'callers' instead of 'refs'
      // (especially since macros cannot have overrides)

      std::string decl_usr;
      if (cursor.get_kind() == CXCursor_MacroDefinition)
        decl_usr = cursor.get_usr();
      else
        decl_usr = cursor.get_referenced().get_usr();

      IndexVarId var_id = db->ToVarId(decl_usr);
      IndexVar* var_def = db->Resolve(var_id);
      UniqueAdd(var_def->uses, decl_loc_spelling);

      if (cursor.get_kind() == CXCursor_MacroDefinition) {
        CXSourceRange cx_extent = clang_getCursorExtent(cursor.cx_cursor);
        var_def->def.short_name = cursor.get_display_name();
        var_def->def.detailed_name = cursor.get_display_name();
        var_def->def.hover =
            "#define " + GetDocumentContentInRange(param->tu->cx_tu, cx_extent);
        var_def->def.cls = VarClass::Macro;
        var_def->def.definition_spelling = decl_loc_spelling;
        var_def->def.definition_extent = Resolve(cx_extent, nullptr);
      }

      break;
    }
    default:
      break;
  }

  return ClangCursor::VisitResult::Continue;
}

void OnIndexDeclaration(CXClientData client_data, const CXIdxDeclInfo* decl) {
  if (!kIndexStdDeclarations &&
      clang_Location_isInSystemHeader(
          clang_indexLoc_getCXSourceLocation(decl->loc)))
    return;

  IndexParam* param = static_cast<IndexParam*>(client_data);

  // Track all constructor declarations, as we may need to use it to manually
  // associate std::make_unique and the like as constructor invocations.
  if (decl->entityInfo->kind == CXIdxEntity_CXXConstructor) {
    param->ctors.NotifyConstructor(decl->cursor);
  }

  CXFile file;
  clang_getSpellingLocation(clang_indexLoc_getCXSourceLocation(decl->loc),
                            &file, nullptr, nullptr, nullptr);
  IndexFile* db = ConsumeFile(param, file);
  if (!db)
    return;

  // The language of this declaration
  LanguageId decl_lang = [decl]() {
    switch (clang_getCursorLanguage(decl->cursor)) {
      case CXLanguage_C:
        return LanguageId::C;
      case CXLanguage_CPlusPlus:
        return LanguageId::Cpp;
      case CXLanguage_ObjC:
        return LanguageId::ObjC;
      default:
        return LanguageId::Unknown;
    };
  }();

  // Only update the file language if the new language is "greater" than the old
  if (decl_lang > db->language) {
    db->language = decl_lang;
  }

  NamespaceHelper* ns = &param->ns;

  switch (decl->entityInfo->kind) {
    case CXIdxEntity_CXXNamespace: {
      ns->RegisterQualifiedName(decl->entityInfo->USR, decl->semanticContainer,
                                decl->entityInfo->name);
      break;
    }

    case CXIdxEntity_ObjCProperty:
    case CXIdxEntity_ObjCIvar:
    case CXIdxEntity_EnumConstant:
    case CXIdxEntity_Field:
    case CXIdxEntity_Variable:
    case CXIdxEntity_CXXStaticVariable: {
      Range decl_loc_spelling = ResolveSpelling(decl->cursor);

      ClangCursor decl_cursor = decl->cursor;

      // Do not index implicit template instantiations.
      if (decl_cursor !=
          decl_cursor.template_specialization_to_template_definition())
        break;

      std::string decl_usr = decl_cursor.get_usr();

      IndexVarId var_id = db->ToVarId(decl->entityInfo->USR);
      IndexVar* var = db->Resolve(var_id);

      // TODO: Eventually run with this if. Right now I want to iron out bugs
      // this may shadow.
      // TODO: Verify this gets called multiple times
      // if (!decl->isRedeclaration) {
      var->def.short_name = decl->entityInfo->name;

      std::string type_name =
          ToString(clang_getTypeSpelling(clang_getCursorType(decl->cursor)));
      // clang may report "(lambda at foo.cc)" which end up being a very long
      // string. Shorten it to just "lambda".
      if (type_name.find("(lambda at") != std::string::npos)
        type_name = "lambda";

      {
        std::string qualified_name =
            ns->QualifiedName(decl->semanticContainer, var->def.short_name);
        if (decl->entityInfo->kind == CXIdxEntity_EnumConstant)
          var->def.detailed_name = std::move(qualified_name);
        else
          var->def.detailed_name = type_name + " " + std::move(qualified_name);
      }

      bool is_system = clang_Location_isInSystemHeader(
          clang_indexLoc_getCXSourceLocation(decl->loc));
      if (is_system)
        var->def.cls = VarClass::Unknown;
      else {
        if (IsGlobalContainer(decl->semanticContainer))
          var->def.cls = VarClass::Global;
        else if (IsTypeDefinition(decl->semanticContainer))
          var->def.cls = VarClass::Member;
        else
          var->def.cls = VarClass::Local;
      }
      //}

      if (decl->isDefinition) {
        var->def.definition_spelling = ResolveSpelling(decl->cursor);
        var->def.definition_extent = ResolveExtent(decl->cursor);
        ;
      } else {
        var->def.declaration = ResolveSpelling(decl->cursor);
      }
      UniqueAdd(var->uses, decl_loc_spelling);

      AddDeclInitializerUsages(db, decl_cursor);
      var = db->Resolve(var_id);

      // Declaring variable type information. Note that we do not insert an
      // interesting reference for parameter declarations - that is handled when
      // the function declaration is encountered since we won't receive ParmDecl
      // declarations for unnamed parameters.
      // TODO: See if we can remove this function call.
      AddDeclTypeUsages(db, decl_cursor, decl->semanticContainer,
                        decl->lexicalContainer);

      // We don't need to assign declaring type multiple times if this variable
      // has already been seen.
      if (!decl->isRedeclaration) {
        optional<IndexTypeId> var_type =
            ResolveToDeclarationType(db, decl_cursor);
        if (var_type.has_value()) {
          // Don't treat enum definition variables as instantiations.
          bool is_enum_member =
              decl->semanticContainer &&
              decl->semanticContainer->cursor.kind == CXCursor_EnumDecl;
          if (!is_enum_member)
            db->Resolve(var_type.value())->instances.push_back(var_id);

          var->def.variable_type = var_type.value();
        }
      }

      // TODO: Refactor handlers so more things are under 'if
      // (!decl->isRedeclaration)'
      if (decl->isDefinition && IsTypeDefinition(decl->semanticContainer)) {
        IndexTypeId declaring_type_id =
            db->ToTypeId(decl->semanticContainer->cursor);
        IndexType* declaring_type_def = db->Resolve(declaring_type_id);
        var->def.declaring_type = declaring_type_id;
        declaring_type_def->def.vars.push_back(var_id);
      }

      break;
    }

    case CXIdxEntity_ObjCInstanceMethod:
    case CXIdxEntity_ObjCClassMethod:
    case CXIdxEntity_Function:
    case CXIdxEntity_CXXConstructor:
    case CXIdxEntity_CXXDestructor:
    case CXIdxEntity_CXXInstanceMethod:
    case CXIdxEntity_CXXStaticMethod:
    case CXIdxEntity_CXXConversionFunction: {
      Range decl_spelling = ResolveSpelling(decl->cursor);
      Range decl_extent = ResolveExtent(decl->cursor);

      ClangCursor decl_cursor = decl->cursor;
      ClangCursor decl_cursor_resolved =
          decl_cursor.template_specialization_to_template_definition();
      bool is_template_specialization = decl_cursor != decl_cursor_resolved;

      IndexFuncId func_id = db->ToFuncId(decl_cursor_resolved.cx_cursor);
      IndexFunc* func = db->Resolve(func_id);

      // We don't actually need to know the return type, but we need to mark it
      // as an interesting usage.
      AddDeclTypeUsages(db, decl_cursor, decl->semanticContainer,
                        decl->lexicalContainer);

      // Add definition or declaration. This is a bit tricky because we treat
      // template specializations as declarations, even though they are
      // technically definitions.
      // TODO: Support multiple function definitions, which is common for
      //       template specializations.
      if (decl->isDefinition && !is_template_specialization) {
        // assert(!func->def.definition_spelling);
        // assert(!func->def.definition_extent);
        func->def.definition_spelling = decl_spelling;
        func->def.definition_extent = decl_extent;
      } else {
        IndexFunc::Declaration declaration;
        declaration.spelling = decl_spelling;
        declaration.extent = decl_extent;
        declaration.content = GetDocumentContentInRange(
            param->tu->cx_tu, clang_getCursorExtent(decl->cursor));

        // Add parameters.
        for (ClangCursor arg : decl_cursor.get_arguments()) {
          switch (arg.get_kind()) {
            case CXCursor_ParmDecl: {
              Range param_spelling = ResolveSpelling(arg.cx_cursor);

              // If the name is empty (which is common for parameters), clang
              // will report a range with length 1, which is not correct.
              if (param_spelling.start.column ==
                      (param_spelling.end.column - 1) &&
                  arg.get_display_name().empty()) {
                param_spelling.end.column -= 1;
              }

              declaration.param_spellings.push_back(param_spelling);
              break;
            }
            default:
              break;
          }
        }

        func->declarations.push_back(declaration);
      }

      // Emit definition data for the function. We do this even if it isn't a
      // definition because there can be, for example, interfaces, or a class
      // declaration that doesn't have a definition yet. If we never end up
      // indexing the definition, then there will not be any (ie) outline
      // information.
      if (!is_template_specialization) {
        func->def.short_name = decl->entityInfo->name;

        // Set the |is_operator| flag to true if the function name starts with
        // "operator"
        func->def.is_operator =
            func->def.short_name.compare(0, 8, "operator") == 0;

        // Build detailed name. The type desc looks like void (void *). We
        // insert the qualified name before the first '('.
        std::string qualified_name =
            ns->QualifiedName(decl->semanticContainer, func->def.short_name);
        std::string type_desc = decl_cursor.get_type_description();
        {
          size_t balance = 0, offset;
          for (offset = type_desc.size(); offset;) {
            offset--;
            if (type_desc[offset] == ')')
              balance++;
            // Balanced paren pair that may appear before the paren enclosing
            // function parameters, see clang/lib/AST/TypePrinter.cpp
            else if (type_desc[offset] == '(' && --balance == 0 &&
                     !((offset >= 5 &&
                        !type_desc.compare(offset - 5, 5, "throw")) ||
                       (offset >= 6 &&
                        !type_desc.compare(offset - 6, 6, "typeof")) ||
                       (offset >= 7 &&
                        !type_desc.compare(offset - 7, 7, "_Atomic")) ||
                       (offset >= 8 &&
                        !type_desc.compare(offset - 8, 8, "decltype")) ||
                       (offset >= 8 &&
                        !type_desc.compare(offset - 8, 8, "noexcept")) ||
                       (offset >= 13 &&
                        !type_desc.compare(offset - 13, 13, "__attribute__"))))
              break;
          }
          if (offset > 0) {
            type_desc.insert(offset, qualified_name);
            func->def.detailed_name = type_desc;
          } else {
            // type_desc is probably the name of a typedef.
            func->def.detailed_name = type_desc + " " + qualified_name;
          }
        }

        // Add function usage information. We only want to do it once per
        // definition/declaration. Do it on definition since there should only
        // ever be one of those in the entire program.
        if (IsTypeDefinition(decl->semanticContainer)) {
          IndexTypeId declaring_type_id =
              db->ToTypeId(decl->semanticContainer->cursor);
          IndexType* declaring_type_def = db->Resolve(declaring_type_id);
          func->def.declaring_type = declaring_type_id;

          // Mark a type reference at the ctor/dtor location.
          if (decl->entityInfo->kind == CXIdxEntity_CXXConstructor)
            UniqueAdd(declaring_type_def->uses, decl_spelling);
          if (decl->entityInfo->kind == CXIdxEntity_CXXDestructor) {
            Range dtor_type_range = decl_spelling;
            dtor_type_range.start.column += 1;  // Don't count the leading ~
            UniqueAdd(declaring_type_def->uses, dtor_type_range);
          }

          // Add function to declaring type.
          UniqueAdd(declaring_type_def->def.funcs, func_id);
        }

        // Process inheritance.
        if (clang_CXXMethod_isVirtual(decl->cursor)) {
          CXCursor* overridden;
          unsigned int num_overridden;
          clang_getOverriddenCursors(decl->cursor, &overridden,
                                     &num_overridden);

          for (unsigned i = 0; i < num_overridden; ++i) {
            ClangCursor parent =
                ClangCursor(overridden[i])
                    .template_specialization_to_template_definition();
            IndexFuncId parent_id = db->ToFuncId(parent.get_usr());
            IndexFunc* parent_def = db->Resolve(parent_id);
            func = db->Resolve(func_id);  // ToFuncId invalidated func_def

            func->def.base.push_back(parent_id);
            parent_def->derived.push_back(func_id);
          }

          clang_disposeOverriddenCursors(overridden);
        }
      }
      break;
    }

    case CXIdxEntity_Typedef:
    case CXIdxEntity_CXXTypeAlias: {
      Range decl_loc_spelling = ResolveSpelling(decl->cursor);

      // Note we want to fetch the first TypeRef. Running
      // ResolveCursorType(decl->cursor) would return
      // the type of the typedef/using, not the type of the referenced type.
      optional<IndexTypeId> alias_of = AddDeclTypeUsages(
          db, decl->cursor, decl->semanticContainer, decl->lexicalContainer);

      IndexTypeId type_id = db->ToTypeId(decl->entityInfo->USR);
      IndexType* type = db->Resolve(type_id);

      if (alias_of)
        type->def.alias_of = alias_of.value();

      Range spell = ResolveSpelling(decl->cursor);
      Range extent = ResolveExtent(decl->cursor);
      type->def.definition_spelling = spell;
      type->def.definition_extent = extent;

      type->def.short_name = decl->entityInfo->name;
      type->def.detailed_name =
          ns->QualifiedName(decl->semanticContainer, type->def.short_name);

      // For Typedef/CXXTypeAlias spanning a few lines, display the declaration
      // line, with spelling name replaced with qualified name.
      // TODO Think how to display multi-line declaration like `typedef struct {
      // ... } foo;` https://github.com/jacobdufault/cquery/issues/29
      if (extent.end.line - extent.start.line <
          kMaxLinesDisplayTypeAliasDeclarations) {
        FileContentsWithOffsets& fc = param->file_contents[db->path];
        optional<int> extent_start = fc.ToOffset(extent.start),
                      spell_start = fc.ToOffset(spell.start),
                      spell_end = fc.ToOffset(spell.end),
                      extent_end = fc.ToOffset(extent.end);
        if (extent_start && spell_start && spell_end && extent_end) {
          type->def.hover =
              fc.contents.substr(*extent_start, *spell_start - *extent_start) +
              type->def.detailed_name +
              fc.contents.substr(*spell_end, *extent_end - *spell_end);
        }
      }

      UniqueAdd(type->uses, decl_loc_spelling);
      break;
    }

    case CXIdxEntity_ObjCProtocol:
    case CXIdxEntity_ObjCCategory:
    case CXIdxEntity_ObjCClass:
    case CXIdxEntity_Enum:
    case CXIdxEntity_Union:
    case CXIdxEntity_Struct:
    case CXIdxEntity_CXXClass: {
      Range decl_loc_spelling = ResolveSpelling(decl->cursor);

      IndexTypeId type_id = db->ToTypeId(decl->entityInfo->USR);
      IndexType* type = db->Resolve(type_id);

      // TODO: Eventually run with this if. Right now I want to iron out bugs
      // this may shadow.
      // TODO: For type section, verify if this ever runs for non definitions?
      // if (!decl->isRedeclaration) {

      // name can be null in an anonymous struct (see
      // tests/types/anonymous_struct.cc).
      if (decl->entityInfo->name) {
        ns->RegisterQualifiedName(decl->entityInfo->USR,
                                  decl->semanticContainer,
                                  decl->entityInfo->name);
        type->def.short_name = decl->entityInfo->name;
      } else {
        type->def.short_name = "<anonymous>";
      }

      type->def.detailed_name =
          ns->QualifiedName(decl->semanticContainer, type->def.short_name);
      // }

      if (decl->isDefinition) {
        type->def.definition_spelling = ResolveSpelling(decl->cursor);
        type->def.definition_extent = ResolveExtent(decl->cursor);
      }
      UniqueAdd(type->uses, decl_loc_spelling);

      // type_def->alias_of
      // type_def->funcs
      // type_def->types
      // type_def->uses
      // type_def->vars

      // Add type-level inheritance information.
      CXIdxCXXClassDeclInfo const* class_info =
          clang_index_getCXXClassDeclInfo(decl);
      if (class_info) {
        for (unsigned int i = 0; i < class_info->numBases; ++i) {
          const CXIdxBaseClassInfo* base_class = class_info->bases[i];

          AddDeclTypeUsages(db, base_class->cursor, decl->semanticContainer,
                            decl->lexicalContainer);
          optional<IndexTypeId> parent_type_id =
              ResolveToDeclarationType(db, base_class->cursor);
          // type_def ptr could be invalidated by ResolveToDeclarationType.
          type = db->Resolve(type_id);
          if (parent_type_id) {
            IndexType* parent_type_def = db->Resolve(parent_type_id.value());
            parent_type_def->derived.push_back(type_id);
            type->def.parents.push_back(parent_type_id.value());
          }
        }
      }
      break;
    }

    default:
      std::cerr << "!! Unhandled indexDeclaration:     "
                << ClangCursor(decl->cursor).ToString() << " at "
                << ResolveSpelling(decl->cursor).start.ToString() << std::endl;
      std::cerr << "     entityInfo->kind  = " << decl->entityInfo->kind
                << std::endl;
      std::cerr << "     entityInfo->USR   = " << decl->entityInfo->USR
                << std::endl;
      if (decl->declAsContainer)
        std::cerr << "     declAsContainer   = "
                  << ClangCursor(decl->declAsContainer->cursor).ToString()
                  << std::endl;
      if (decl->semanticContainer)
        std::cerr << "     semanticContainer = "
                  << ClangCursor(decl->semanticContainer->cursor).ToString()
                  << std::endl;
      if (decl->lexicalContainer)
        std::cerr << "     lexicalContainer  = "
                  << ClangCursor(decl->lexicalContainer->cursor).get_usr()
                  << std::endl;
      break;
  }
}

bool IsFunctionCallContext(CXCursorKind kind) {
  switch (kind) {
    case CXCursor_FunctionDecl:
    case CXCursor_CXXMethod:
    case CXCursor_Constructor:
    case CXCursor_Destructor:
    case CXCursor_ConversionFunction:
    case CXCursor_FunctionTemplate:
    case CXCursor_OverloadedDeclRef:
    // TODO: we need to test lambdas
    case CXCursor_LambdaExpr:
      return true;

    default:
      break;
  }

  return false;
}

void OnIndexReference(CXClientData client_data, const CXIdxEntityRefInfo* ref) {
  // TODO: Use clang_getFileUniqueID
  CXFile file;
  clang_getSpellingLocation(clang_indexLoc_getCXSourceLocation(ref->loc), &file,
                            nullptr, nullptr, nullptr);
  IndexParam* param = static_cast<IndexParam*>(client_data);
  IndexFile* db = ConsumeFile(param, file);
  if (!db)
    return;

  ClangCursor cursor(ref->cursor);

  switch (ref->referencedEntity->kind) {
    case CXIdxEntity_CXXNamespaceAlias:
    case CXIdxEntity_CXXNamespace: {
      // We don't index namespace usages.
      break;
    }

    case CXIdxEntity_ObjCProperty:
    case CXIdxEntity_ObjCIvar:
    case CXIdxEntity_EnumConstant:
    case CXIdxEntity_CXXStaticVariable:
    case CXIdxEntity_Variable:
    case CXIdxEntity_Field: {
      Range loc_spelling = ResolveSpelling(ref->cursor);

      ClangCursor referenced = ref->referencedEntity->cursor;
      referenced = referenced.template_specialization_to_template_definition();

      IndexVarId var_id = db->ToVarId(referenced.get_usr());
      IndexVar* var = db->Resolve(var_id);
      // Lambda paramaters are not processed by OnIndexDeclaration and
      // may not have a short_name yet. Note that we only process the lambda
      // parameter as a definition if it is in the same file as the reference,
      // as lambdas cannot be split across files.
      if (var->def.short_name.empty()) {
        CXFile referenced_file;
        Range spelling =
            ResolveSpelling(referenced.cx_cursor, &referenced_file);
        if (file == referenced_file) {
          var->def.definition_spelling = spelling;
          var->def.definition_extent = ResolveExtent(referenced.cx_cursor);

          // TODO Some of the logic here duplicates CXIdxEntity_Variable branch
          // of OnIndexDeclaration. But there `decl` is of type CXIdxDeclInfo
          // and has more information, thus not easy to reuse the code.
          var->def.short_name = referenced.get_spelling();
          std::string type_name = ToString(
              clang_getTypeSpelling(clang_getCursorType(referenced.cx_cursor)));
          var->def.detailed_name = type_name + " " + var->def.short_name;
          var->def.cls = VarClass::Member;
          UniqueAdd(var->uses, ResolveSpelling(referenced.cx_cursor));
          AddDeclInitializerUsages(db, referenced.cx_cursor);
          // TODO Use proper semantic_container and lexical_container.
          AddDeclTypeUsages(db, referenced.cx_cursor, nullptr, nullptr);
          // TODO Other logic in OnIndexDeclaration may need to be adapted.
        }
      }
      UniqueAdd(var->uses, loc_spelling);
      break;
    }

    case CXIdxEntity_CXXConversionFunction:
    case CXIdxEntity_CXXStaticMethod:
    case CXIdxEntity_CXXInstanceMethod:
    case CXIdxEntity_ObjCInstanceMethod:
    case CXIdxEntity_ObjCClassMethod:
    case CXIdxEntity_Function:
    case CXIdxEntity_CXXConstructor:
    case CXIdxEntity_CXXDestructor: {
      // TODO: Redirect container to constructor for the following example, ie,
      //       we should be inserting an outgoing function call from the Foo
      //       ctor.
      //
      //  int Gen() { return 5; }
      //  class Foo {
      //    int x = Gen();
      //  }

      // TODO: search full history?
      Range loc_spelling = ResolveSpelling(ref->cursor);

      IndexFuncId called_id = db->ToFuncId(ref->referencedEntity->USR);
      IndexFunc* called = db->Resolve(called_id);

      // libclang doesn't provide a nice api to check if the given function
      // call is implicit. ref->kind should probably work (it's either direct
      // or implicit), but libclang only supports implicit for objective-c.
      bool is_implicit =
          CanBeCalledImplicitly(ref->referencedEntity->kind) &&
          // Treats empty short_name as an implicit call like implicit move
          // constructor in `vector<int> a = f();`
          (called->def.short_name.empty() ||
           // For explicit destructor call, ref->cursor may be "~" while
           // called->def.short_name is "~A"
           // "~A" is not a substring of ref->cursor, but we should take this
           // case as not `is_implicit`.
           (called->def.short_name[0] != '~' &&
            !CursorSpellingContainsString(ref->cursor, param->tu->cx_tu,
                                          called->def.short_name)));

      if (IsFunctionCallContext(ref->container->cursor.kind)) {
        IndexFuncId caller_id = db->ToFuncId(ref->container->cursor);
        IndexFunc* caller = db->Resolve(caller_id);
        // Calling db->ToFuncId invalidates the FuncDef* ptrs.
        called = db->Resolve(called_id);

        AddFuncRef(&caller->def.callees,
                   IndexFuncRef(called_id, loc_spelling, is_implicit));
        AddFuncRef(&called->callers,
                   IndexFuncRef(caller_id, loc_spelling, is_implicit));
      } else {
        AddFuncRef(&called->callers, IndexFuncRef(loc_spelling, is_implicit));
      }

      // Checks if |str| starts with |start|. Ignores case.
      auto str_begin = [](const char* start, const char* str) {
        while (*start && *str) {
          char a = tolower(*start);
          char b = tolower(*str);
          if (a != b)
            return false;
          ++start;
          ++str;
        }
        return !*start;
      };

      bool is_template = ref->referencedEntity->templateKind !=
                         CXIdxEntityCXXTemplateKind::CXIdxEntity_NonTemplate;
      if (is_template && str_begin("make", ref->referencedEntity->name)) {
        // Try to find the return type of called function. That type will have
        // the constructor function we add a usage to.
        optional<ClangCursor> opt_found_type = FindType(ref->cursor);
        if (opt_found_type) {
          std::string ctor_type_usr =
              opt_found_type->get_referenced().get_usr();
          ClangCursor call_cursor = ref->cursor;

          // Build a type description from the parameters of the call, so we
          // can try to find a constructor with the same type description.
          std::vector<std::string> call_type_desc;
          for (ClangType type : call_cursor.get_type().get_arguments()) {
            std::string type_desc = type.get_spelling();
            if (!type_desc.empty())
              call_type_desc.push_back(type_desc);
          }

          // Try to find the constructor and add a reference.
          optional<std::string> ctor_usr =
              param->ctors.TryFindConstructorUsr(ctor_type_usr, call_type_desc);
          if (ctor_usr) {
            IndexFunc* ctor = db->Resolve(db->ToFuncId(*ctor_usr));
            AddFuncRef(&ctor->callers,
                       IndexFuncRef(loc_spelling, true /*is_implicit*/));
          }
        }
      }

      break;
    }

    case CXIdxEntity_ObjCCategory:
    case CXIdxEntity_ObjCProtocol:
    case CXIdxEntity_ObjCClass:
    case CXIdxEntity_Typedef:
    case CXIdxEntity_CXXTypeAlias:
    case CXIdxEntity_Enum:
    case CXIdxEntity_Union:
    case CXIdxEntity_Struct:
    case CXIdxEntity_CXXClass: {
      ClangCursor referenced_cursor = ref->referencedEntity->cursor;
      referenced_cursor =
          referenced_cursor.template_specialization_to_template_definition();
      IndexTypeId referenced_id = db->ToTypeId(referenced_cursor.get_usr());

      IndexType* referenced = db->Resolve(referenced_id);

      //
      // The following will generate two TypeRefs to Foo, both located at the
      // same spot (line 3, column 3). One of the parents will be set to
      // CXIdxEntity_Variable, the other will be CXIdxEntity_Function. There
      // does not appear to be a good way to disambiguate these references, as
      // using parent type alone breaks other indexing tasks.
      //
      // To work around this, we check to see if the usage location has been
      // inserted into all_uses previously.
      //
      //  struct Foo {};
      //  void Make() {
      //    Foo f;
      //  }
      //
      UniqueAdd(referenced->uses, ResolveSpelling(ref->cursor));
      break;
    }

    default:
      std::cerr << "!! Unhandled indexEntityReference: " << cursor.ToString()
                << " at " << ResolveSpelling(ref->cursor).start.ToString()
                << std::endl;
      std::cerr << "     ref->referencedEntity->kind = "
                << ref->referencedEntity->kind << std::endl;
      if (ref->parentEntity)
        std::cerr << "     ref->parentEntity->kind = "
                  << ref->parentEntity->kind << std::endl;
      std::cerr << "     ref->loc          = "
                << ResolveSpelling(ref->cursor).start.ToString() << std::endl;
      std::cerr << "     ref->kind         = " << ref->kind << std::endl;
      if (ref->parentEntity)
        std::cerr << "     parentEntity      = "
                  << ClangCursor(ref->parentEntity->cursor).ToString()
                  << std::endl;
      if (ref->referencedEntity)
        std::cerr << "     referencedEntity  = "
                  << ClangCursor(ref->referencedEntity->cursor).ToString()
                  << std::endl;
      if (ref->container)
        std::cerr << "     container         = "
                  << ClangCursor(ref->container->cursor).ToString()
                  << std::endl;
      break;
  }
}

FileContents::FileContents(const std::string& path, const std::string& content)
    : path(path), content(content) {}

FileContentsWithOffsets::FileContentsWithOffsets() : line_offsets_{0} {}

FileContentsWithOffsets::FileContentsWithOffsets(std::string s) {
  contents = s;
  line_offsets_.push_back(0);
  for (size_t i = 0; i < s.size(); i++)
    if (s[i] == '\n')
      line_offsets_.push_back(i + 1);
}

optional<int> FileContentsWithOffsets::ToOffset(Position p) const {
  if (0 < p.line && size_t(p.line) <= line_offsets_.size()) {
    int ret = line_offsets_[p.line - 1] + p.column - 1;
    if (size_t(ret) <= contents.size())
      return {ret};
  }
  return nullopt;
}

optional<std::string> FileContentsWithOffsets::ContentsInRange(
    Range range) const {
  optional<int> start_offset = ToOffset(range.start),
                end_offset = ToOffset(range.end);
  if (start_offset && end_offset && *start_offset < *end_offset)
    return {contents.substr(*start_offset, *end_offset - *start_offset)};
  return nullopt;
}

std::vector<std::unique_ptr<IndexFile>> Parse(
    Config* config,
    FileConsumer::SharedState* file_consumer_shared,
    std::string file,
    const std::vector<std::string>& args,
    const std::vector<FileContents>& file_contents,
    PerformanceImportFile* perf,
    ClangIndex* index,
    bool dump_ast) {
  if (!config->enableIndexing)
    return {};

  file = NormalizePath(file);

  Timer timer;

  std::vector<CXUnsavedFile> unsaved_files;
  for (const FileContents& contents : file_contents) {
    CXUnsavedFile unsaved;
    unsaved.Filename = contents.path.c_str();
    unsaved.Contents = contents.content.c_str();
    unsaved.Length = (unsigned long)contents.content.size();
    unsaved_files.push_back(unsaved);
  }

  std::unique_ptr<ClangTranslationUnit> tu = ClangTranslationUnit::Create(
      index, file, args, unsaved_files,
      CXTranslationUnit_KeepGoing |
          CXTranslationUnit_DetailedPreprocessingRecord);
  if (!tu)
    return {};

  perf->index_parse = timer.ElapsedMicrosecondsAndReset();

  if (dump_ast)
    Dump(clang_getTranslationUnitCursor(tu->cx_tu));

  return ParseWithTu(file_consumer_shared, perf, tu.get(), index, file, args,
                     unsaved_files);
}

std::vector<std::unique_ptr<IndexFile>> ParseWithTu(
    FileConsumer::SharedState* file_consumer_shared,
    PerformanceImportFile* perf,
    ClangTranslationUnit* tu,
    ClangIndex* index,
    const std::string& file,
    const std::vector<std::string>& args,
    const std::vector<CXUnsavedFile>& file_contents) {
  Timer timer;

  IndexerCallbacks callback = {0};
  // Available callbacks:
  // - abortQuery
  // - enteredMainFile
  // - ppIncludedFile
  // - importedASTFile
  // - startedTranslationUnit
  callback.diagnostic = &OnIndexDiagnostic;
  callback.ppIncludedFile = &OnIndexIncludedFile;
  callback.indexDeclaration = &OnIndexDeclaration;
  callback.indexEntityReference = &OnIndexReference;

  FileConsumer file_consumer(file_consumer_shared, file);
  IndexParam param(tu, &file_consumer);
  for (const CXUnsavedFile& contents : file_contents) {
    param.file_contents.emplace(
        contents.Filename, std::string(contents.Contents, contents.Length));
  }

  CXFile cx_file = clang_getFile(tu->cx_tu, file.c_str());
  param.primary_file = ConsumeFile(&param, cx_file);

  CXIndexAction index_action = clang_IndexAction_create(index->cx_index);

  // NOTE: libclang re-enables crash recovery whenever a new index is created.
  // To have clang crash toggle crash recovery right before calling
  // clang_indexTranslationUnit.
  // clang_toggleCrashRecovery(0);

  // |index_result| is a CXErrorCode instance.
  int index_result = clang_indexTranslationUnit(
      index_action, &param, &callback, sizeof(IndexerCallbacks),
      CXIndexOpt_IndexFunctionLocalSymbols |
          CXIndexOpt_SkipParsedBodiesInSession |
          CXIndexOpt_IndexImplicitTemplateInstantiations,
      tu->cx_tu);
  if (index_result != CXError_Success) {
    LOG_S(WARNING) << "Indexing " << file
                   << " failed with errno=" << index_result;
    return {};
  }

  clang_IndexAction_dispose(index_action);

  ClangCursor(clang_getTranslationUnitCursor(tu->cx_tu))
      .VisitChildren(&VisitMacroDefinitionAndExpansions, &param);

  perf->index_build = timer.ElapsedMicrosecondsAndReset();

  auto result = param.file_consumer->TakeLocalState();
  for (std::unique_ptr<IndexFile>& entry : result) {
    entry->import_file = file;
    entry->args = args;

    // Update file contents and modification time.
    entry->file_contents_ = param.file_contents[entry->path].contents;
    entry->last_modification_time = param.file_modification_times[entry->path];

    // Update dependencies for the file. Do not include the file in its own
    // dependency set.
    entry->dependencies = param.seen_files;
    entry->dependencies.erase(
        std::remove(entry->dependencies.begin(), entry->dependencies.end(),
                    entry->path),
        entry->dependencies.end());

    // Make sure we are using correct file contents.
    for (const CXUnsavedFile& contents : file_contents) {
      if (entry->path == contents.Filename)
        entry->file_contents_ = std::string(contents.Contents, contents.Length);
    }
  }

  return result;
}

void IndexInit() {
  clang_enableStackTraces();
  clang_toggleCrashRecovery(1);
}

void ClangSanityCheck() {
  std::vector<const char*> args = {"clang", "tests/vars/class_member.cc"};
  unsigned opts = 0;
  CXIndex index = clang_createIndex(0, 1);
  CXTranslationUnit tu;
  clang_parseTranslationUnit2FullArgv(index, nullptr, args.data(), args.size(),
                                      nullptr, 0, opts, &tu);
  assert(tu);

  IndexerCallbacks callback = {0};
  callback.abortQuery = [](CXClientData client_data, void* reserved) {
    return 0;
  };
  callback.diagnostic = [](CXClientData client_data,
                           CXDiagnosticSet diagnostics, void* reserved) {};
  callback.enteredMainFile = [](CXClientData client_data, CXFile mainFile,
                                void* reserved) -> CXIdxClientFile {
    return nullptr;
  };
  callback.ppIncludedFile = [](
      CXClientData client_data,
      const CXIdxIncludedFileInfo* file) -> CXIdxClientFile { return nullptr; };
  callback.importedASTFile = [](
      CXClientData client_data,
      const CXIdxImportedASTFileInfo*) -> CXIdxClientASTFile {
    return nullptr;
  };
  callback.startedTranslationUnit = [](CXClientData client_data,
                                       void* reserved) -> CXIdxClientContainer {
    return nullptr;
  };
  callback.indexDeclaration = [](CXClientData client_data,
                                 const CXIdxDeclInfo* decl) {};
  callback.indexEntityReference = [](CXClientData client_data,
                                     const CXIdxEntityRefInfo* ref) {};

  const unsigned kIndexOpts = 0;
  CXIndexAction index_action = clang_IndexAction_create(index);
  int index_param = 0;
  clang_toggleCrashRecovery(0);
  clang_indexTranslationUnit(index_action, &index_param, &callback,
                             sizeof(IndexerCallbacks), kIndexOpts, tu);
  clang_IndexAction_dispose(index_action);

  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(index);
}

std::string GetClangVersion() {
  return ToString(clang_getClangVersion());
}
