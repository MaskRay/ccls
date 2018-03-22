#include "indexer.h"

#include "clang_cursor.h"
#include "clang_utils.h"
#include "platform.h"
#include "serializer.h"
#include "timer.h"
#include "type_printer.h"

#include <loguru.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <iostream>

// TODO: See if we can use clang_indexLoc_getFileLocation to get a type ref on
// |Foobar| in DISALLOW_COPY(Foobar)

#if CINDEX_VERSION >= 47
#define CINDEX_HAVE_PRETTY 1
#endif
#if CINDEX_VERSION >= 48
#define CINDEX_HAVE_ROLE 1
#endif

namespace {

// For typedef/using spanning less than or equal to (this number) of lines,
// display their declarations on hover.
constexpr int kMaxLinesDisplayTypeAliasDeclarations = 3;

// TODO How to check if a reference to type is a declaration?
// This currently also includes constructors/destructors.
// It seems declarations in functions are not indexed.
bool IsDeclContext(CXIdxEntityKind kind) {
  switch (kind) {
    case CXIdxEntity_CXXClass:
    case CXIdxEntity_CXXNamespace:
    case CXIdxEntity_ObjCCategory:
    case CXIdxEntity_ObjCClass:
    case CXIdxEntity_ObjCProtocol:
    case CXIdxEntity_Struct:
      return true;
    default:
      return false;
  }
}

Role GetRole(const CXIdxEntityRefInfo* ref_info, Role role) {
#if CINDEX_HAVE_ROLE
  return static_cast<Role>(static_cast<int>(ref_info->role));
#else
  return role;
#endif
}

SymbolKind GetSymbolKind(CXCursorKind kind) {
  switch (kind) {
    case CXCursor_TranslationUnit:
      return SymbolKind::File;

    case CXCursor_FunctionDecl:
    case CXCursor_CXXMethod:
    case CXCursor_Constructor:
    case CXCursor_Destructor:
    case CXCursor_ConversionFunction:
    case CXCursor_FunctionTemplate:
    case CXCursor_OverloadedDeclRef:
    case CXCursor_LambdaExpr:
    case CXCursor_ObjCInstanceMethodDecl:
    case CXCursor_ObjCClassMethodDecl:
      return SymbolKind::Func;

    case CXCursor_StructDecl:
    case CXCursor_UnionDecl:
    case CXCursor_ClassDecl:
    case CXCursor_EnumDecl:
    case CXCursor_ObjCInterfaceDecl:
    case CXCursor_ObjCCategoryDecl:
    case CXCursor_ObjCImplementationDecl:
    case CXCursor_Namespace:
      return SymbolKind::Type;

    default:
      return SymbolKind::Invalid;
  }
}

// Inverse of libclang/CXIndexDataConsumer.cpp getEntityKindFromSymbolKind
lsSymbolKind GetSymbolKind(CXIdxEntityKind kind) {
  switch (kind) {
    case CXIdxEntity_Unexposed:
      return lsSymbolKind::Unknown;
    case CXIdxEntity_Typedef:
      return lsSymbolKind::TypeAlias;
    case CXIdxEntity_Function:
      return lsSymbolKind::Function;
    case CXIdxEntity_Variable:
      // Can also be Parameter
      return lsSymbolKind::Variable;
    case CXIdxEntity_Field:
      return lsSymbolKind::Field;
    case CXIdxEntity_EnumConstant:
      return lsSymbolKind::EnumMember;

    case CXIdxEntity_ObjCClass:
      return lsSymbolKind::Class;
    case CXIdxEntity_ObjCProtocol:
      return lsSymbolKind::Interface;
    case CXIdxEntity_ObjCCategory:
      return lsSymbolKind::Interface;

    case CXIdxEntity_ObjCInstanceMethod:
      return lsSymbolKind::Method;
    case CXIdxEntity_ObjCClassMethod:
      return lsSymbolKind::StaticMethod;
    case CXIdxEntity_ObjCProperty:
      return lsSymbolKind::Property;
    case CXIdxEntity_ObjCIvar:
      return lsSymbolKind::Field;

    case CXIdxEntity_Enum:
      return lsSymbolKind::Enum;
    case CXIdxEntity_Struct:
    case CXIdxEntity_Union:
      return lsSymbolKind::Struct;

    case CXIdxEntity_CXXClass:
      return lsSymbolKind::Class;
    case CXIdxEntity_CXXNamespace:
      return lsSymbolKind::Namespace;
    case CXIdxEntity_CXXNamespaceAlias:
      return lsSymbolKind::Namespace;
    case CXIdxEntity_CXXStaticVariable:
      return lsSymbolKind::Field;
    case CXIdxEntity_CXXStaticMethod:
      return lsSymbolKind::StaticMethod;
    case CXIdxEntity_CXXInstanceMethod:
      return lsSymbolKind::Method;
    case CXIdxEntity_CXXConstructor:
      return lsSymbolKind::Constructor;
    case CXIdxEntity_CXXDestructor:
      return lsSymbolKind::Method;
    case CXIdxEntity_CXXConversionFunction:
      return lsSymbolKind::Constructor;
    case CXIdxEntity_CXXTypeAlias:
      return lsSymbolKind::TypeAlias;
    case CXIdxEntity_CXXInterface:
      return lsSymbolKind::Struct;
  }

  return lsSymbolKind::Unknown;
}

StorageClass GetStorageClass(CX_StorageClass storage) {
  switch (storage) {
    case CX_SC_Invalid:
    case CX_SC_OpenCLWorkGroupLocal:
      return StorageClass::Invalid;
    case CX_SC_None:
      return StorageClass::None;
    case CX_SC_Extern:
      return StorageClass::Extern;
    case CX_SC_Static:
      return StorageClass::Static;
    case CX_SC_PrivateExtern:
      return StorageClass::PrivateExtern;
    case CX_SC_Auto:
      return StorageClass::Auto;
    case CX_SC_Register:
      return StorageClass::Register;
  }

  return StorageClass::None;
}

// Caches all instances of constructors, regardless if they are indexed or not.
// The constructor may have a make_unique call associated with it that we need
// to export. If we do not capture the parameter type description for the
// constructor we will not be able to attribute the constructor call correctly.
struct ConstructorCache {
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

    Constructor ctor{ctor_cursor.get_usr_hash(), build_type_desc(ctor_cursor)};

    // Insert into |constructors_|.
    auto type_usr_hash = ctor_cursor.get_semantic_parent().get_usr_hash();
    auto existing_ctors = constructors_.find(type_usr_hash);
    if (existing_ctors != constructors_.end()) {
      existing_ctors->second.push_back(ctor);
    } else {
      constructors_[type_usr_hash] = {ctor};
    }
  }

  // Tries to lookup a constructor in |type_usr| that takes arguments most
  // closely aligned to |param_type_desc|.
  optional<Usr> TryFindConstructorUsr(
      Usr type_usr,
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

    Usr best_usr = ctors[0].usr;
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
      for (size_t i = 0;
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

    return best_usr;
  }
};

struct IndexParam {
  Config* config = nullptr;

  std::unordered_set<CXFile> seen_cx_files;
  std::vector<std::string> seen_files;
  FileContentsMap file_contents;
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

  IndexParam(Config* config,
             ClangTranslationUnit* tu,
             FileConsumer* file_consumer)
      : config(config), tu(tu), file_consumer(file_consumer) {}

#if CINDEX_HAVE_PRETTY
  CXPrintingPolicy print_policy = nullptr;
  CXPrintingPolicy print_policy_more = nullptr;
  ~IndexParam() {
    clang_PrintingPolicy_dispose(print_policy);
    clang_PrintingPolicy_dispose(print_policy_more);
  }

  std::string PrettyPrintCursor(CXCursor cursor, bool initializer = true) {
    if (!print_policy) {
      print_policy = clang_getCursorPrintingPolicy(cursor);
      clang_PrintingPolicy_setProperty(print_policy,
                                       CXPrintingPolicy_TerseOutput, 1);
      clang_PrintingPolicy_setProperty(print_policy,
                                       CXPrintingPolicy_FullyQualifiedName, 1);
      clang_PrintingPolicy_setProperty(
          print_policy, CXPrintingPolicy_SuppressInitializers, 1);
      print_policy_more = clang_getCursorPrintingPolicy(cursor);
      clang_PrintingPolicy_setProperty(print_policy_more,
                                       CXPrintingPolicy_FullyQualifiedName, 1);
      clang_PrintingPolicy_setProperty(print_policy_more,
                                       CXPrintingPolicy_TerseOutput, 1);
    }
    return ToString(clang_getCursorPrettyPrinted(
        cursor, initializer ? print_policy_more : print_policy));
  }
#endif
};

IndexFile* ConsumeFile(IndexParam* param, CXFile file) {
  bool is_first_ownership = false;
  IndexFile* db = param->file_consumer->TryConsumeFile(
      file, &is_first_ownership, &param->file_contents);

  // If this is the first time we have seen the file (ignoring if we are
  // generating an index for it):
  if (param->seen_cx_files.insert(file).second) {
    std::string file_name = FileName(file);
    // file_name may be empty when it contains .. and is outside of WorkingDir.
    // https://reviews.llvm.org/D42893
    // https://github.com/cquery-project/cquery/issues/413
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
    }
  }

  if (is_first_ownership) {
    // Report skipped source range list.
    CXSourceRangeList* skipped = clang_getSkippedRanges(param->tu->cx_tu, file);
    for (unsigned i = 0; i < skipped->count; ++i) {
      Range range = ResolveCXSourceRange(skipped->ranges[i]);
#if CINDEX_VERSION < 45  // Before clang 6.0.0
      // clang_getSkippedRanges reports start one token after the '#', move it
      // back so it starts at the '#'
      range.start.column -= 1;
#endif
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
                                  std::string_view needle) {
  CXSourceRange range = clang_Cursor_getSpellingNameRange(cursor, 0, 0);
  CXToken* tokens;
  unsigned num_tokens;
  clang_tokenize(cx_tu, range, &tokens, &num_tokens);

  bool result = false;

  for (unsigned i = 0; i < num_tokens; ++i) {
    CXString name = clang_getTokenSpelling(cx_tu, tokens[i]);
    if (needle == clang_getCString(name)) {
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
    Range token_range =
        ResolveCXSourceRange(clang_getTokenExtent(cx_tu, tokens[i]));
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

void SetUsePreflight(IndexFile* db, ClangCursor parent) {
  switch (GetSymbolKind(parent.get_kind())) {
    case SymbolKind::Func:
      (void)db->ToFuncId(parent.cx_cursor);
      break;
    case SymbolKind::Type:
      (void)db->ToTypeId(parent.cx_cursor);
      break;
    case SymbolKind::Var:
      (void)db->ToVarId(parent.cx_cursor);
      break;
    default:
      break;
  }
}

// |parent| should be resolved before using |SetUsePreflight| so that |def| will
// not be invalidated by |To{Func,Type,Var}Id|.
Use SetUse(IndexFile* db, Range range, ClangCursor parent, Role role) {
  switch (GetSymbolKind(parent.get_kind())) {
    case SymbolKind::Func:
      return Use(range, db->ToFuncId(parent.cx_cursor), SymbolKind::Func, role,
                 {});
    case SymbolKind::Type:
      return Use(range, db->ToTypeId(parent.cx_cursor), SymbolKind::Type, role,
                 {});
    case SymbolKind::Var:
      return Use(range, db->ToVarId(parent.cx_cursor), SymbolKind::Var, role,
                 {});
    default:
      return Use(range, Id<void>(), SymbolKind::File, role, {});
  }
}

const char* GetAnonName(CXCursorKind kind) {
  switch (kind) {
    case CXCursor_ClassDecl:
      return "(anon class)";
    case CXCursor_EnumDecl:
      return "(anon enum)";
    case CXCursor_StructDecl:
      return "(anon struct)";
    case CXCursor_UnionDecl:
      return "(anon union)";
    default:
      return "(anon)";
  }
}

void SetTypeName(IndexType* type,
                 const ClangCursor& cursor,
                 const CXIdxContainerInfo* container,
                 const char* name,
                 IndexParam* param) {
  CXIdxContainerInfo parent;
  // |name| can be null in an anonymous struct (see
  // tests/types/anonymous_struct.cc).
  if (!name)
    name = GetAnonName(cursor.get_kind());
  if (!container)
    parent.cursor = cursor.get_semantic_parent().cx_cursor;
  // Investigate why clang_getCursorPrettyPrinted gives `struct A {}` `namespace
  // ns {}` which are not qualified.
  // type->def.detailed_name = param->PrettyPrintCursor(cursor.cx_cursor);
  type->def.detailed_name =
      param->ns.QualifiedName(container ? container : &parent, name);
  auto idx = type->def.detailed_name.rfind(name);
  assert(idx != std::string::npos);
  type->def.short_name_offset = idx;
  type->def.short_name_size = strlen(name);
}

// Finds the cursor associated with the declaration type of |cursor|. This
// strips
// qualifies from |cursor| (ie, Foo* => Foo) and removes template arguments
// (ie, Foo<A,B> => Foo<*,*>).
optional<IndexTypeId> ResolveToDeclarationType(IndexFile* db,
                                               ClangCursor cursor,
                                               IndexParam* param) {
  ClangType type = cursor.get_type();

  // auto x = new Foo() will not be deduced to |Foo| if we do not use the
  // canonical type. However, a canonical type will look past typedefs so we
  // will not accurately report variables on typedefs if we always do this.
  if (type.cx_type.kind == CXType_Auto)
    type = type.get_canonical();

  type = type.strip_qualifiers();

  if (type.is_builtin()) {
    // For builtin types, use type kinds as USR hash.
    return db->ToTypeId(type.cx_type.kind);
  }

  ClangCursor declaration =
      type.get_declaration().template_specialization_to_template_definition();
  CXString cx_usr = clang_getCursorUSR(declaration.cx_cursor);
  const char* str_usr = clang_getCString(cx_usr);
  if (!str_usr || str_usr[0] == '\0') {
    clang_disposeString(cx_usr);
    return nullopt;
  }
  Usr usr = HashUsr(str_usr);
  clang_disposeString(cx_usr);
  IndexTypeId type_id = db->ToTypeId(usr);
  IndexType* typ = db->Resolve(type_id);
  if (typ->def.detailed_name.empty()) {
    std::string name = declaration.get_spell_name();
    SetTypeName(typ, declaration, nullptr, name.c_str(), param);
  }
  return type_id;
}

void SetVarDetail(IndexVar* var,
                  std::string_view short_name,
                  const ClangCursor& cursor,
                  const CXIdxContainerInfo* semanticContainer,
                  bool is_first_seen,
                  IndexFile* db,
                  IndexParam* param) {
  IndexVar::Def& def = var->def;
  const CXType cx_type = clang_getCursorType(cursor.cx_cursor);
  std::string type_name = ToString(clang_getTypeSpelling(cx_type));
  // clang may report "(lambda at foo.cc)" which end up being a very long
  // string. Shorten it to just "lambda".
  if (type_name.find("(lambda at") != std::string::npos)
    type_name = "lambda";
  if (param->config->index.comments)
    def.comments = cursor.get_comments();
  def.storage = GetStorageClass(clang_Cursor_getStorageClass(cursor.cx_cursor));

  // TODO how to make PrettyPrint'ed variable name qualified?
  std::string qualified_name =
#if 0 && CINDEX_HAVE_PRETTY
      cursor.get_kind() != CXCursor_EnumConstantDecl
          ? param->PrettyPrintCursor(cursor.cx_cursor)
          :
#endif
      param->ns.QualifiedName(semanticContainer, short_name);

  if (cursor.get_kind() == CXCursor_EnumConstantDecl && semanticContainer) {
    CXType enum_type = clang_getCanonicalType(
        clang_getEnumDeclIntegerType(semanticContainer->cursor));
    std::string hover = qualified_name + " = ";
    if (enum_type.kind == CXType_UInt || enum_type.kind == CXType_ULong ||
        enum_type.kind == CXType_ULongLong)
      hover += std::to_string(
          clang_getEnumConstantDeclUnsignedValue(cursor.cx_cursor));
    else
      hover += std::to_string(clang_getEnumConstantDeclValue(cursor.cx_cursor));
    def.detailed_name = std::move(qualified_name);
    def.hover = hover;
  } else {
#if 0 && CINDEX_HAVE_PRETTY
    //def.detailed_name = param->PrettyPrintCursor(cursor.cx_cursor, false);
#else
    ConcatTypeAndName(type_name, qualified_name);
    def.detailed_name = type_name;
    // Append the textual initializer, bit field, constructor to |hover|.
    // Omit |hover| for these types:
    // int (*a)(); int (&a)(); int (&&a)(); int a[1]; auto x = ...
    // We can take these into consideration after we have better support for
    // inside-out syntax.
    CXType deref = cx_type;
    while (deref.kind == CXType_Pointer || deref.kind == CXType_MemberPointer ||
           deref.kind == CXType_LValueReference ||
           deref.kind == CXType_RValueReference)
      deref = clang_getPointeeType(deref);
    if (deref.kind != CXType_Unexposed && deref.kind != CXType_Auto &&
        clang_getResultType(deref).kind == CXType_Invalid &&
        clang_getElementType(deref).kind == CXType_Invalid) {
      const FileContents& fc = param->file_contents[db->path];
      optional<int> spell_end = fc.ToOffset(cursor.get_spell().end);
      optional<int> extent_end = fc.ToOffset(cursor.get_extent().end);
      if (extent_end && *spell_end < *extent_end)
        def.hover = std::string(def.detailed_name.c_str()) +
                    fc.content.substr(*spell_end, *extent_end - *spell_end);
    }
#endif
  }
  // FIXME QualifiedName should return index
  auto idx = def.detailed_name.rfind(short_name.begin(), std::string::npos,
                                     short_name.size());
  assert(idx != std::string::npos);
  def.short_name_offset = idx;
  def.short_name_size = short_name.size();

  if (is_first_seen) {
    optional<IndexTypeId> var_type =
        ResolveToDeclarationType(db, cursor, param);
    if (var_type) {
      // Don't treat enum definition variables as instantiations.
      bool is_enum_member = semanticContainer &&
                            semanticContainer->cursor.kind == CXCursor_EnumDecl;
      if (!is_enum_member)
        db->Resolve(var_type.value())->instances.push_back(var->id);

      def.type = *var_type;
    }
  }
}

void OnIndexReference_Function(IndexFile* db,
                               Range loc,
                               ClangCursor parent_cursor,
                               IndexFuncId called_id,
                               Role role) {
  switch (GetSymbolKind(parent_cursor.get_kind())) {
    case SymbolKind::Func: {
      IndexFunc* parent = db->Resolve(db->ToFuncId(parent_cursor.cx_cursor));
      IndexFunc* called = db->Resolve(called_id);
      parent->def.callees.push_back(
          SymbolRef(loc, called->id, SymbolKind::Func, role));
      called->uses.push_back(Use(loc, parent->id, SymbolKind::Func, role, {}));
      break;
    }
    case SymbolKind::Type: {
      IndexType* parent = db->Resolve(db->ToTypeId(parent_cursor.cx_cursor));
      IndexFunc* called = db->Resolve(called_id);
      called = db->Resolve(called_id);
      called->uses.push_back(Use(loc, parent->id, SymbolKind::Type, role, {}));
      break;
    }
    default: {
      IndexFunc* called = db->Resolve(called_id);
      called->uses.push_back(Use(loc, Id<void>(), SymbolKind::File, role, {}));
      break;
    }
  }
}

}  // namespace

// static
const int IndexFile::kMajorVersion = 15;
const int IndexFile::kMinorVersion = 0;

IndexFile::IndexFile(const std::string& path, const std::string& contents)
    : id_cache(path), path(path), file_contents(contents) {}

IndexTypeId IndexFile::ToTypeId(Usr usr) {
  auto it = id_cache.usr_to_type_id.find(usr);
  if (it != id_cache.usr_to_type_id.end())
    return it->second;

  IndexTypeId id(types.size());
  types.push_back(IndexType(id, usr));
  id_cache.usr_to_type_id[usr] = id;
  id_cache.type_id_to_usr[id] = usr;
  return id;
}
IndexFuncId IndexFile::ToFuncId(Usr usr) {
  auto it = id_cache.usr_to_func_id.find(usr);
  if (it != id_cache.usr_to_func_id.end())
    return it->second;

  IndexFuncId id(funcs.size());
  funcs.push_back(IndexFunc(id, usr));
  id_cache.usr_to_func_id[usr] = id;
  id_cache.func_id_to_usr[id] = usr;
  return id;
}
IndexVarId IndexFile::ToVarId(Usr usr) {
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
  return ToTypeId(ClangCursor(cursor).get_usr_hash());
}

IndexFuncId IndexFile::ToFuncId(const CXCursor& cursor) {
  return ToFuncId(ClangCursor(cursor).get_usr_hash());
}

IndexVarId IndexFile::ToVarId(const CXCursor& cursor) {
  return ToVarId(ClangCursor(cursor).get_usr_hash());
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
  return Serialize(SerializeFormat::Json, *this);
}

IndexType::IndexType(IndexTypeId id, Usr usr) : usr(usr), id(id) {}

template <typename T>
void Uniquify(std::vector<Id<T>>& ids) {
  std::unordered_set<Id<T>> seen;
  size_t n = 0;
  for (size_t i = 0; i < ids.size(); i++)
    if (seen.insert(ids[i]).second)
      ids[n++] = ids[i];
  ids.resize(n);
}

void Uniquify(std::vector<Use>& uses) {
  std::unordered_set<Range> seen;
  size_t n = 0;
  for (size_t i = 0; i < uses.size(); i++)
    if (seen.insert(uses[i].range).second)
      uses[n++] = uses[i];
  uses.resize(n);
}

// FIXME Reference: set id in call sites and remove this
// void AddUse(std::vector<Use>& values, Range value) {
//  values.push_back(
//      Use(value, Id<void>(), SymbolKind::File, Role::Reference, {}));
//}

void AddUse(IndexFile* db,
            std::vector<Use>& uses,
            Range range,
            ClangCursor parent,
            Role role = Role::Reference) {
  switch (GetSymbolKind(parent.get_kind())) {
    case SymbolKind::Func:
      uses.push_back(Use(range, db->ToFuncId(parent.cx_cursor),
                         SymbolKind::Func, role, {}));
      break;
    case SymbolKind::Type:
      uses.push_back(Use(range, db->ToTypeId(parent.cx_cursor),
                         SymbolKind::Type, role, {}));
      break;
    default:
      uses.push_back(Use(range, Id<void>(), SymbolKind::File, role, {}));
      break;
  }
}

CXCursor fromContainer(const CXIdxContainerInfo* parent) {
  return parent ? parent->cursor : clang_getNullCursor();
}

void AddUseSpell(IndexFile* db, std::vector<Use>& uses, ClangCursor cursor) {
  AddUse(db, uses, cursor.get_spell(), cursor.get_lexical_parent().cx_cursor);
}

IdCache::IdCache(const std::string& primary_file)
    : primary_file(primary_file) {}

void OnIndexDiagnostic(CXClientData client_data,
                       CXDiagnosticSet diagnostics,
                       void* reserved) {
  IndexParam* param = static_cast<IndexParam*>(client_data);

  for (unsigned i = 0; i < clang_getNumDiagnosticsInSet(diagnostics); ++i) {
    CXDiagnostic diagnostic = clang_getDiagnosticInSet(diagnostics, i);

    CXSourceLocation diag_loc = clang_getDiagnosticLocation(diagnostic);
    // Skip diagnostics in system headers.
    // if (clang_Location_isInSystemHeader(diag_loc))
    //   continue;

    // Get db so we can attribute diagnostic to the right indexed file.
    CXFile file;
    unsigned int line, column;
    clang_getSpellingLocation(diag_loc, &file, &line, &column, nullptr);
    // Skip empty diagnostic.
    if (!line && !column)
      continue;
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
  line--;

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
  std::cerr << ToString(cursor.get_kind()) << " " << cursor.get_spell_name()
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

bool IsTypeDefinition(const CXIdxContainerInfo* container) {
  if (!container)
    return false;
  return GetSymbolKind(container->cursor.kind) == SymbolKind::Type;
}

struct VisitDeclForTypeUsageParam {
  IndexFile* db;
  optional<IndexTypeId> toplevel_type;
  int has_processed_any = false;
  optional<ClangCursor> previous_cursor;
  optional<IndexTypeId> initial_type;

  VisitDeclForTypeUsageParam(IndexFile* db, optional<IndexTypeId> toplevel_type)
      : db(db), toplevel_type(toplevel_type) {}
};

void VisitDeclForTypeUsageVisitorHandler(ClangCursor cursor,
                                         VisitDeclForTypeUsageParam* param) {
  param->has_processed_any = true;
  IndexFile* db = param->db;

  // For |A<int> a| where there is a specialization for |A<int>|,
  // the |referenced_usr| below resolves to the primary template and
  // attributes the use to the primary template instead of the specialization.
  // |toplevel_type| is retrieved |clang_getCursorType| which can be a
  // specialization. If its name is the same as the primary template's, we
  // assume the use should be attributed to the specialization. This heuristic
  // fails when a member class bears the same name with its container.
  //
  // template<class T>
  // struct C { struct C {}; };
  // C<int>::C a;
  //
  // We will attribute |::C| to the parent class.
  if (param->toplevel_type) {
    IndexType* ref_type = db->Resolve(*param->toplevel_type);
    std::string name = cursor.get_referenced().get_spell_name();
    if (name == ref_type->def.ShortName()) {
      AddUseSpell(db, ref_type->uses, cursor);
      param->toplevel_type = nullopt;
      return;
    }
  }

  std::string referenced_usr =
      cursor.get_referenced()
          .template_specialization_to_template_definition()
          .get_usr();
  // TODO: things in STL cause this to be empty. Figure out why and document it.
  if (referenced_usr == "")
    return;

  IndexTypeId ref_type_id = db->ToTypeId(HashUsr(referenced_usr));

  if (!param->initial_type)
    param->initial_type = ref_type_id;

  IndexType* ref_type_def = db->Resolve(ref_type_id);
  // TODO: Should we even be visiting this if the file is not from the main
  // def? Try adding assert on |loc| later.
  AddUseSpell(db, ref_type_def->uses, cursor);
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

// Add usages to any seen TypeRef or TemplateRef under the given |decl_cursor|.
// This returns the first seen TypeRef or TemplateRef value, which can be
// useful if trying to figure out ie, what a using statement refers to. If
// trying to generally resolve a cursor to a type, use
// ResolveToDeclarationType, which works in more scenarios.
// If |decl_cursor| is a variable of a template type, clang_getCursorType
// may return a specialized template which is preciser than the primary
// template.
// We use |toplevel_type| to attribute the use to the specialized template
// instead of the primary template.
optional<IndexTypeId> AddDeclTypeUsages(
    IndexFile* db,
    ClangCursor decl_cursor,
    optional<IndexTypeId> toplevel_type,
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

  VisitDeclForTypeUsageParam param(db, toplevel_type);
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

  if (param.initial_type)
    return param.initial_type;
  CXType cx_under = clang_getTypedefDeclUnderlyingType(decl_cursor.cx_cursor);
  if (cx_under.kind == CXType_Invalid)
    return nullopt;
  return db->ToTypeId(ClangType(cx_under).strip_qualifiers().get_usr_hash());
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
      // different Usr.

      // ClangCursor ref =
      // cursor.get_referenced().template_specialization_to_template_definition().get_type().strip_qualifiers().get_usr_hash();
      // std::string ref_usr =
      // cursor.get_referenced().template_specialization_to_template_definition().get_type().strip_qualifiers().get_usr_hash();
      auto ref_usr = cursor.get_referenced()
                         .template_specialization_to_template_definition()
                         .get_usr();
      // std::string ref_usr = ref.get_usr_hash();
      if (ref_usr == "")
        break;

      IndexVar* ref_var = db->Resolve(db->ToVarId(HashUsr(ref_usr)));
      AddUseSpell(db, ref_var->uses, cursor);
      break;
    }

    default:
      break;
  }

  return ClangCursor::VisitResult::Recurse;
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
      CXFile file;
      Range decl_loc_spelling = ResolveCXSourceRange(cx_source_range, &file);
      IndexFile* db = ConsumeFile(param, file);
      if (!db)
        break;

      // TODO: Considering checking clang_Cursor_isMacroFunctionLike, but the
      // only real difference will be that we show 'callers' instead of 'refs'
      // (especially since macros cannot have overrides)

      Usr decl_usr;
      if (cursor.get_kind() == CXCursor_MacroDefinition)
        decl_usr = cursor.get_usr_hash();
      else
        decl_usr = cursor.get_referenced().get_usr_hash();

      SetUsePreflight(db, parent);
      IndexVar* var_def = db->Resolve(db->ToVarId(decl_usr));
      if (cursor.get_kind() == CXCursor_MacroDefinition) {
        CXSourceRange cx_extent = clang_getCursorExtent(cursor.cx_cursor);
        var_def->def.detailed_name = cursor.get_display_name();
        var_def->def.short_name_offset = 0;
        var_def->def.short_name_size =
            int16_t(strlen(var_def->def.detailed_name.c_str()));
        var_def->def.hover =
            "#define " + GetDocumentContentInRange(param->tu->cx_tu, cx_extent);
        var_def->def.kind = lsSymbolKind::Macro;
        if (param->config->index.comments)
          var_def->def.comments = cursor.get_comments();
        var_def->def.spell =
            SetUse(db, decl_loc_spelling, parent, Role::Definition);
        var_def->def.extent = SetUse(
            db, ResolveCXSourceRange(cx_extent, nullptr), parent, Role::None);
      } else
        AddUse(db, var_def->uses, decl_loc_spelling, parent);

      break;
    }
    default:
      break;
  }

  return ClangCursor::VisitResult::Continue;
}

namespace {

// TODO Move to another file and use clang C++ API
struct TemplateVisitorData {
  IndexFile* db;
  IndexParam* param;
  ClangCursor container;
};

ClangCursor::VisitResult TemplateVisitor(ClangCursor cursor,
                                         ClangCursor parent,
                                         TemplateVisitorData* data) {
  IndexFile* db = data->db;
  IndexParam* param = data->param;
  switch (cursor.get_kind()) {
    default:
      break;
    case CXCursor_DeclRefExpr: {
      ClangCursor ref_cursor = clang_getCursorReferenced(cursor.cx_cursor);
      if (ref_cursor.get_kind() == CXCursor_NonTypeTemplateParameter) {
        IndexVarId ref_var_id = db->ToVarId(ref_cursor.get_usr_hash());
        IndexVar* ref_var = db->Resolve(ref_var_id);
        if (ref_var->def.detailed_name.empty()) {
          ClangCursor sem_parent = ref_cursor.get_semantic_parent();
          ClangCursor lex_parent = ref_cursor.get_lexical_parent();
          SetUsePreflight(db, sem_parent);
          SetUsePreflight(db, lex_parent);
          ref_var = db->Resolve(ref_var_id);
          ref_var->def.spell =
              SetUse(db, ref_cursor.get_spell(), sem_parent, Role::Definition);
          ref_var->def.extent =
              SetUse(db, ref_cursor.get_extent(), lex_parent, Role::None);
          ref_var = db->Resolve(ref_var_id);
          ref_var->def.kind = lsSymbolKind::TypeParameter;
          SetVarDetail(ref_var, ref_cursor.get_spell_name(), ref_cursor,
                       nullptr, true, db, param);

          ClangType ref_type = clang_getCursorType(ref_cursor.cx_cursor);
          // TODO optimize
          if (ref_type.get_usr().size()) {
            IndexType* ref_type_index =
                db->Resolve(db->ToTypeId(ref_type.get_usr_hash()));
            // The cursor extent includes `type name`, not just `name`. There
            // seems no way to extract the spelling range of `type` and we do
            // not want to do subtraction here.
            // See https://github.com/jacobdufault/cquery/issues/252
            AddUse(db, ref_type_index->uses, ref_cursor.get_extent(),
                   ref_cursor.get_lexical_parent());
          }
        }
        AddUseSpell(db, ref_var->uses, cursor);
      }
      break;
    }
    case CXCursor_OverloadedDeclRef: {
      unsigned num_overloaded = clang_getNumOverloadedDecls(cursor.cx_cursor);
      for (unsigned i = 0; i != num_overloaded; i++) {
        ClangCursor overloaded = clang_getOverloadedDecl(cursor.cx_cursor, i);
        switch (overloaded.get_kind()) {
          default:
            break;
          case CXCursor_FunctionDecl:
          case CXCursor_FunctionTemplate: {
            IndexFuncId called_id = db->ToFuncId(overloaded.get_usr_hash());
            OnIndexReference_Function(db, cursor.get_spell(), data->container,
                                      called_id, Role::Call);
            break;
          }
        }
      }
      break;
    }
    case CXCursor_TemplateRef: {
      ClangCursor ref_cursor = clang_getCursorReferenced(cursor.cx_cursor);
      if (ref_cursor.get_kind() == CXCursor_TemplateTemplateParameter) {
        IndexTypeId ref_type_id = db->ToTypeId(ref_cursor.get_usr_hash());
        IndexType* ref_type = db->Resolve(ref_type_id);
        // TODO It seems difficult to get references to template template
        // parameters.
        // CXCursor_TemplateTemplateParameter can be visited by visiting
        // CXCursor_TranslationUnit, but not (confirm this) by visiting
        // {Class,Function}Template. Thus we need to initialize it here.
        if (ref_type->def.detailed_name.empty()) {
          ClangCursor sem_parent = ref_cursor.get_semantic_parent();
          ClangCursor lex_parent = ref_cursor.get_lexical_parent();
          SetUsePreflight(db, sem_parent);
          SetUsePreflight(db, lex_parent);
          ref_type = db->Resolve(ref_type_id);
          ref_type->def.spell =
              SetUse(db, ref_cursor.get_spell(), sem_parent, Role::Definition);
          ref_type->def.extent =
              SetUse(db, ref_cursor.get_extent(), lex_parent, Role::None);
#if 0 && CINDEX_HAVE_PRETTY
          ref_type->def.detailed_name = param->PrettyPrintCursor(ref_cursor.cx_cursor);
#else
          ref_type->def.detailed_name = ref_cursor.get_spell_name();
#endif
          ref_type->def.short_name_offset = 0;
          ref_type->def.short_name_size =
              int16_t(strlen(ref_type->def.detailed_name.c_str()));
          ref_type->def.kind = lsSymbolKind::TypeParameter;
        }
        AddUseSpell(db, ref_type->uses, cursor);
      }
      break;
    }
    case CXCursor_TypeRef: {
      ClangCursor ref_cursor = clang_getCursorReferenced(cursor.cx_cursor);
      if (ref_cursor.get_kind() == CXCursor_TemplateTypeParameter) {
        IndexTypeId ref_type_id = db->ToTypeId(ref_cursor.get_usr_hash());
        IndexType* ref_type = db->Resolve(ref_type_id);
        // TODO It seems difficult to get a FunctionTemplate's template
        // parameters.
        // CXCursor_TemplateTypeParameter can be visited by visiting
        // CXCursor_TranslationUnit, but not (confirm this) by visiting
        // {Class,Function}Template. Thus we need to initialize it here.
        if (ref_type->def.detailed_name.empty()) {
          ClangCursor sem_parent = ref_cursor.get_semantic_parent();
          ClangCursor lex_parent = ref_cursor.get_lexical_parent();
          SetUsePreflight(db, sem_parent);
          SetUsePreflight(db, lex_parent);
          ref_type = db->Resolve(ref_type_id);
          ref_type->def.spell =
              SetUse(db, ref_cursor.get_spell(), sem_parent, Role::Definition);
          ref_type->def.extent =
              SetUse(db, ref_cursor.get_extent(), lex_parent, Role::None);
#if 0 && CINDEX_HAVE_PRETTY
          // template<class T> void f(T t){} // weird, the name is empty
          ref_type->def.detailed_name = param->PrettyPrintCursor(ref_cursor.cx_cursor);
#else
          ref_type->def.detailed_name = ref_cursor.get_spell_name();
#endif
          ref_type->def.short_name_offset = 0;
          ref_type->def.short_name_size =
              int16_t(strlen(ref_type->def.detailed_name.c_str()));
          ref_type->def.kind = lsSymbolKind::TypeParameter;
        }
        AddUseSpell(db, ref_type->uses, cursor);
      }
      break;
    }
  }
  return ClangCursor::VisitResult::Recurse;
}

}  // namespace

std::string NamespaceHelper::QualifiedName(const CXIdxContainerInfo* container,
                                           std::string_view unqualified_name) {
  if (!container)
    return std::string(unqualified_name);
  // Anonymous namespaces are not processed by indexDeclaration. We trace
  // nested namespaces bottom-up through clang_getCursorSemanticParent until
  // one that we know its qualified name. Then do another trace top-down and
  // put their names into a map of USR -> qualified_name.
  ClangCursor cursor(container->cursor);
  std::vector<ClangCursor> namespaces;
  std::string qualifier;
  while (cursor.get_kind() != CXCursor_TranslationUnit &&
         GetSymbolKind(cursor.get_kind()) == SymbolKind::Type) {
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
    std::string name = namespaces[i].get_spell_name();
    // Empty name indicates unnamed namespace, anonymous struct, anonymous
    // union, ...
    if (name.size())
      qualifier += name;
    else
      qualifier += GetAnonName(namespaces[i].get_kind());
    qualifier += "::";
    container_cursor_to_qualified_name[namespaces[i]] = qualifier;
  }
  // C++17 string::append
  return qualifier + std::string(unqualified_name);
}

void OnIndexDeclaration(CXClientData client_data, const CXIdxDeclInfo* decl) {
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
  LanguageId decl_lang = [&decl]() {
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

  ClangCursor sem_parent(fromContainer(decl->semanticContainer));
  ClangCursor lex_parent(fromContainer(decl->lexicalContainer));
  SetUsePreflight(db, sem_parent);
  SetUsePreflight(db, lex_parent);
  ClangCursor cursor = decl->cursor;

  switch (decl->entityInfo->kind) {
    case CXIdxEntity_Unexposed:
      LOG_S(INFO) << "CXIdxEntity_Unexposed " << cursor.get_spell_name();
      break;

    case CXIdxEntity_CXXNamespace: {
      Range spell = cursor.get_spell();
      IndexTypeId ns_id = db->ToTypeId(HashUsr(decl->entityInfo->USR));
      IndexType* ns = db->Resolve(ns_id);
      ns->def.kind = GetSymbolKind(decl->entityInfo->kind);
      if (ns->def.detailed_name.empty()) {
        SetTypeName(ns, cursor, decl->semanticContainer, decl->entityInfo->name,
                    param);
        ns->def.spell = SetUse(db, spell, sem_parent, Role::Definition);
        ns->def.extent =
            SetUse(db, cursor.get_extent(), lex_parent, Role::None);
        if (decl->semanticContainer) {
          IndexTypeId parent_id = db->ToTypeId(
              ClangCursor(decl->semanticContainer->cursor).get_usr_hash());
          db->Resolve(parent_id)->derived.push_back(ns_id);
          // |ns| may be invalidated.
          ns = db->Resolve(ns_id);
          ns->def.bases.push_back(parent_id);
        }
      }
      AddUse(db, ns->uses, spell, lex_parent);
      break;
    }

    case CXIdxEntity_CXXNamespaceAlias:
      assert(false && "CXXNamespaceAlias");
      break;

    case CXIdxEntity_ObjCProperty:
    case CXIdxEntity_ObjCIvar:
    case CXIdxEntity_EnumConstant:
    case CXIdxEntity_Field:
    case CXIdxEntity_Variable:
    case CXIdxEntity_CXXStaticVariable: {
      Range spell = cursor.get_spell();

      // Do not index implicit template instantiations.
      if (cursor != cursor.template_specialization_to_template_definition())
        break;

      IndexVarId var_id = db->ToVarId(HashUsr(decl->entityInfo->USR));
      IndexVar* var = db->Resolve(var_id);

      // TODO: Eventually run with this if. Right now I want to iron out bugs
      // this may shadow.
      // TODO: Verify this gets called multiple times
      // if (!decl->isRedeclaration) {
      SetVarDetail(var, std::string(decl->entityInfo->name), decl->cursor,
                   decl->semanticContainer, !decl->isRedeclaration, db, param);

      // FIXME https://github.com/jacobdufault/cquery/issues/239
      var->def.kind = GetSymbolKind(decl->entityInfo->kind);
      if (var->def.kind == lsSymbolKind::Variable &&
          decl->cursor.kind == CXCursor_ParmDecl)
        var->def.kind = lsSymbolKind::Parameter;
      //}

      if (decl->isDefinition) {
        var->def.spell = SetUse(db, spell, sem_parent, Role::Definition);
        var->def.extent =
            SetUse(db, cursor.get_extent(), lex_parent, Role::None);
      } else {
        var->declarations.push_back(
            SetUse(db, spell, lex_parent, Role::Declaration));
      }

      cursor.VisitChildren(&AddDeclInitializerUsagesVisitor, db);
      var = db->Resolve(var_id);

      // Declaring variable type information. Note that we do not insert an
      // interesting reference for parameter declarations - that is handled when
      // the function declaration is encountered since we won't receive ParmDecl
      // declarations for unnamed parameters.
      // TODO: See if we can remove this function call.
      AddDeclTypeUsages(db, cursor, var->def.type, decl->semanticContainer,
                        decl->lexicalContainer);

      // We don't need to assign declaring type multiple times if this variable
      // has already been seen.

      if (decl->isDefinition && decl->semanticContainer) {
        switch (GetSymbolKind(decl->semanticContainer->cursor.kind)) {
          case SymbolKind::Func: {
            db->Resolve(db->ToFuncId(decl->semanticContainer->cursor))
                ->def.vars.push_back(var_id);
            break;
          }
          case SymbolKind::Type:
            if (decl->semanticContainer->cursor.kind != CXCursor_EnumDecl) {
              db->Resolve(db->ToTypeId(decl->semanticContainer->cursor))
                  ->def.vars.push_back(var_id);
            }
            break;
          default:
            break;
        }
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
      Range spell = cursor.get_spell();
      Range extent = cursor.get_extent();

      ClangCursor decl_cursor_resolved =
          cursor.template_specialization_to_template_definition();
      bool is_template_specialization = cursor != decl_cursor_resolved;

      IndexFuncId func_id = db->ToFuncId(decl_cursor_resolved.cx_cursor);
      IndexFunc* func = db->Resolve(func_id);
      if (param->config->index.comments)
        func->def.comments = cursor.get_comments();
      func->def.kind = GetSymbolKind(decl->entityInfo->kind);
      func->def.storage =
          GetStorageClass(clang_Cursor_getStorageClass(decl->cursor));

      // We don't actually need to know the return type, but we need to mark it
      // as an interesting usage.
      AddDeclTypeUsages(db, cursor, nullopt, decl->semanticContainer,
                        decl->lexicalContainer);

      // Add definition or declaration. This is a bit tricky because we treat
      // template specializations as declarations, even though they are
      // technically definitions.
      // TODO: Support multiple function definitions, which is common for
      //       template specializations.
      if (decl->isDefinition && !is_template_specialization) {
        // assert(!func->def.spell);
        // assert(!func->def.extent);
        func->def.spell = SetUse(db, spell, sem_parent, Role::Definition);
        func->def.extent = SetUse(db, extent, lex_parent, Role::None);
      } else {
        IndexFunc::Declaration declaration;
        declaration.spell = SetUse(db, spell, lex_parent, Role::Declaration);

        // Add parameters.
        for (ClangCursor arg : cursor.get_arguments()) {
          switch (arg.get_kind()) {
            case CXCursor_ParmDecl: {
              Range param_spelling = arg.get_spell();

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
      // Build detailed name. The type desc looks like void (void *). We
      // insert the qualified name before the first '('.
      // FIXME GetFunctionSignature should set index
#if CINDEX_HAVE_PRETTY
        func->def.detailed_name = param->PrettyPrintCursor(decl->cursor);
#else
        func->def.detailed_name = GetFunctionSignature(db, &param->ns, decl);
#endif
        auto idx = func->def.detailed_name.find(decl->entityInfo->name);
        assert(idx != std::string::npos);
        func->def.short_name_offset = idx;
        func->def.short_name_size = strlen(decl->entityInfo->name);

        // CXCursor_OverloadedDeclRef in templates are not processed by
        // OnIndexReference, thus we use TemplateVisitor to collect function
        // references.
        if (decl->entityInfo->templateKind == CXIdxEntity_Template) {
          TemplateVisitorData data;
          data.db = db;
          data.param = param;
          data.container = cursor;
          cursor.VisitChildren(&TemplateVisitor, &data);
          // TemplateVisitor calls ToFuncId which invalidates func
          func = db->Resolve(func_id);
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
            AddUse(db, declaring_type_def->uses, spell,
                   fromContainer(decl->lexicalContainer));

          // Add function to declaring type.
          declaring_type_def->def.funcs.push_back(func_id);
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
            IndexFuncId parent_id = db->ToFuncId(parent.get_usr_hash());
            IndexFunc* parent_def = db->Resolve(parent_id);
            func = db->Resolve(func_id);  // ToFuncId invalidated func_def

            func->def.bases.push_back(parent_id);
            parent_def->derived.push_back(func_id);
          }

          clang_disposeOverriddenCursors(overridden);
        }
      }
      break;
    }

    case CXIdxEntity_Typedef:
    case CXIdxEntity_CXXTypeAlias: {
      // Note we want to fetch the first TypeRef. Running
      // ResolveCursorType(decl->cursor) would return
      // the type of the typedef/using, not the type of the referenced type.
      optional<IndexTypeId> alias_of = AddDeclTypeUsages(
          db, cursor, nullopt, decl->semanticContainer, decl->lexicalContainer);

      IndexTypeId type_id = db->ToTypeId(HashUsr(decl->entityInfo->USR));
      IndexType* type = db->Resolve(type_id);

      if (alias_of)
        type->def.alias_of = alias_of.value();

      ClangCursor decl_cursor = decl->cursor;
      Range spell = decl_cursor.get_spell();
      Range extent = decl_cursor.get_extent();
      type->def.spell = SetUse(db, spell, sem_parent, Role::Definition);
      type->def.extent = SetUse(db, extent, lex_parent, Role::None);

      SetTypeName(type, decl_cursor, decl->semanticContainer,
                  decl->entityInfo->name, param);
      type->def.kind = GetSymbolKind(decl->entityInfo->kind);
      if (param->config->index.comments)
        type->def.comments = decl_cursor.get_comments();

      // For Typedef/CXXTypeAlias spanning a few lines, display the declaration
      // line, with spelling name replaced with qualified name.
      // TODO Think how to display multi-line declaration like `typedef struct {
      // ... } foo;` https://github.com/jacobdufault/cquery/issues/29
      if (extent.end.line - extent.start.line <
          kMaxLinesDisplayTypeAliasDeclarations) {
        FileContents& fc = param->file_contents[db->path];
        optional<int> extent_start = fc.ToOffset(extent.start),
                      spell_start = fc.ToOffset(spell.start),
                      spell_end = fc.ToOffset(spell.end),
                      extent_end = fc.ToOffset(extent.end);
        if (extent_start && spell_start && spell_end && extent_end) {
          type->def.hover =
              fc.content.substr(*extent_start, *spell_start - *extent_start) +
              type->def.detailed_name.c_str() +
              fc.content.substr(*spell_end, *extent_end - *spell_end);
        }
      }

      AddUse(db, type->uses, spell, fromContainer(decl->lexicalContainer));
      break;
    }

    case CXIdxEntity_ObjCProtocol:
    case CXIdxEntity_ObjCCategory:
    case CXIdxEntity_ObjCClass:
    case CXIdxEntity_Enum:
    case CXIdxEntity_Union:
    case CXIdxEntity_Struct:
    case CXIdxEntity_CXXInterface:
    case CXIdxEntity_CXXClass: {
      Range spell = cursor.get_spell();

      IndexTypeId type_id = db->ToTypeId(HashUsr(decl->entityInfo->USR));
      IndexType* type = db->Resolve(type_id);

      // TODO: Eventually run with this if. Right now I want to iron out bugs
      // this may shadow.
      // TODO: For type section, verify if this ever runs for non definitions?
      // if (!decl->isRedeclaration) {

      SetTypeName(type, cursor, decl->semanticContainer, decl->entityInfo->name,
                  param);
      type->def.kind = GetSymbolKind(decl->entityInfo->kind);
      if (param->config->index.comments)
        type->def.comments = cursor.get_comments();
      // }

      if (decl->isDefinition) {
        type->def.spell = SetUse(db, spell, sem_parent, Role::Definition);
        type->def.extent =
            SetUse(db, cursor.get_extent(), lex_parent, Role::None);

        if (cursor.get_kind() == CXCursor_EnumDecl) {
          ClangType enum_type = clang_getEnumDeclIntegerType(decl->cursor);
          if (!enum_type.is_builtin()) {
            IndexType* int_type =
                db->Resolve(db->ToTypeId(enum_type.get_usr_hash()));
            AddUse(db, int_type->uses, spell,
                   fromContainer(decl->lexicalContainer));
            // type is invalidated.
            type = db->Resolve(type_id);
          }
        }
      } else
        AddUse(db, type->declarations, spell,
               fromContainer(decl->lexicalContainer), Role::Declaration);

      switch (decl->entityInfo->templateKind) {
        default:
          break;
        case CXIdxEntity_TemplateSpecialization:
        case CXIdxEntity_TemplatePartialSpecialization: {
          // TODO Use a different dimension
          ClangCursor origin_cursor =
              cursor.template_specialization_to_template_definition();
          IndexTypeId origin_id = db->ToTypeId(origin_cursor.get_usr_hash());
          IndexType* origin = db->Resolve(origin_id);
          // |type| may be invalidated.
          type = db->Resolve(type_id);
          // template<class T> class function; // not visited by
          // OnIndexDeclaration template<> class function<int> {}; // current
          // cursor
          if (origin->def.detailed_name.empty()) {
            SetTypeName(origin, origin_cursor, nullptr,
                        &type->def.ShortName()[0], param);
            origin->def.kind = type->def.kind;
          }
          // TODO The name may be assigned in |ResolveToDeclarationType| but
          // |spell| is nullopt.
          CXFile origin_file;
          Range origin_spell = origin_cursor.get_spell(&origin_file);
          if (!origin->def.spell && file == origin_file) {
            ClangCursor origin_sem = origin_cursor.get_semantic_parent();
            ClangCursor origin_lex = origin_cursor.get_lexical_parent();
            SetUsePreflight(db, origin_sem);
            SetUsePreflight(db, origin_lex);
            origin = db->Resolve(origin_id);
            type = db->Resolve(type_id);
            origin->def.spell =
                SetUse(db, origin_spell, origin_sem, Role::Definition);
            origin->def.extent =
                SetUse(db, origin_cursor.get_extent(), origin_lex, Role::None);
          }
          origin->derived.push_back(type_id);
          type->def.bases.push_back(origin_id);
        }
          // fallthrough
        case CXIdxEntity_Template: {
          TemplateVisitorData data;
          data.db = db;
          data.container = cursor;
          data.param = param;
          cursor.VisitChildren(&TemplateVisitor, &data);
          break;
        }
      }

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

          AddDeclTypeUsages(db, base_class->cursor, nullopt,
                            decl->semanticContainer, decl->lexicalContainer);
          optional<IndexTypeId> parent_type_id =
              ResolveToDeclarationType(db, base_class->cursor, param);
          // type_def ptr could be invalidated by ResolveToDeclarationType and
          // TemplateVisitor.
          type = db->Resolve(type_id);
          if (parent_type_id) {
            IndexType* parent_type_def = db->Resolve(parent_type_id.value());
            parent_type_def->derived.push_back(type_id);
            type->def.bases.push_back(*parent_type_id);
          }
        }
      }
      break;
    }
  }
}

// https://github.com/jacobdufault/cquery/issues/174
// Type-dependent member access expressions do not have accurate spelling
// ranges.
//
// Not type dependent
// C<int> f; f.x // .x produces a MemberRefExpr which has a spelling range
// of `x`.
//
// Type dependent
// C<T> e; e.x // .x produces a MemberRefExpr which has a spelling range
// of `e` (weird) and an empty spelling name.
//
// To attribute the use of `x` in `e.x`, we use cursor extent `e.x`
// minus cursor spelling `e` minus the period.
void CheckTypeDependentMemberRefExpr(Range* spell,
                                     const ClangCursor& cursor,
                                     IndexParam* param,
                                     const IndexFile* db) {
  if (cursor.get_kind() == CXCursor_MemberRefExpr &&
      cursor.get_spell_name().empty()) {
    *spell = cursor.get_extent().RemovePrefix(spell->end);
    const FileContents& fc = param->file_contents[db->path];
    optional<int> maybe_period = fc.ToOffset(spell->start);
    if (maybe_period) {
      int i = *maybe_period;
      if (fc.content[i] == '.')
        spell->start.column++;
      // -> is likely unexposed.
    }
  }
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
  ClangCursor lex_parent(fromContainer(ref->container));
  ClangCursor referenced;
  if (ref->referencedEntity)
    referenced = ref->referencedEntity->cursor;
  SetUsePreflight(db, lex_parent);

  switch (ref->referencedEntity->kind) {
    case CXIdxEntity_Unexposed:
      LOG_S(INFO) << "CXIdxEntity_Unexposed " << cursor.get_spell_name();
      break;

    case CXIdxEntity_CXXNamespace: {
      IndexType* ns = db->Resolve(db->ToTypeId(referenced.get_usr_hash()));
      AddUse(db, ns->uses, cursor.get_spell(), fromContainer(ref->container));
      break;
    }

    case CXIdxEntity_CXXNamespaceAlias: {
      IndexType* ns = db->Resolve(db->ToTypeId(referenced.get_usr_hash()));
      AddUse(db, ns->uses, cursor.get_spell(), fromContainer(ref->container));
      if (!ns->def.spell) {
        ClangCursor sem_parent = referenced.get_semantic_parent();
        ClangCursor lex_parent = referenced.get_lexical_parent();
        SetUsePreflight(db, sem_parent);
        SetUsePreflight(db, lex_parent);
        ns->def.spell =
            SetUse(db, referenced.get_spell(), sem_parent, Role::Definition);
        ns->def.extent =
            SetUse(db, referenced.get_extent(), lex_parent, Role::None);
        std::string name = referenced.get_spell_name();
        SetTypeName(ns, referenced, nullptr, name.c_str(), param);
      }
      break;
    }

    case CXIdxEntity_ObjCProperty:
    case CXIdxEntity_ObjCIvar:
    case CXIdxEntity_EnumConstant:
    case CXIdxEntity_CXXStaticVariable:
    case CXIdxEntity_Variable:
    case CXIdxEntity_Field: {
      Range loc = cursor.get_spell();
      CheckTypeDependentMemberRefExpr(&loc, cursor, param, db);

      referenced = referenced.template_specialization_to_template_definition();

      IndexVarId var_id = db->ToVarId(referenced.get_usr_hash());
      IndexVar* var = db->Resolve(var_id);
      // Lambda paramaters are not processed by OnIndexDeclaration and
      // may not have a short_name yet. Note that we only process the lambda
      // parameter as a definition if it is in the same file as the reference,
      // as lambdas cannot be split across files.
      if (var->def.detailed_name.empty()) {
        CXFile referenced_file;
        Range spell = referenced.get_spell(&referenced_file);
        if (file == referenced_file) {
          var->def.spell = SetUse(db, spell, lex_parent, Role::Definition);
          var->def.extent =
              SetUse(db, referenced.get_extent(), lex_parent, Role::None);

          // TODO Some of the logic here duplicates CXIdxEntity_Variable branch
          // of OnIndexDeclaration. But there `decl` is of type CXIdxDeclInfo
          // and has more information, thus not easy to reuse the code.
          SetVarDetail(var, referenced.get_spell_name(), referenced, nullptr,
                       true, db, param);
          var->def.kind = lsSymbolKind::Parameter;
        }
      }
      AddUse(db, var->uses, loc, fromContainer(ref->container),
             GetRole(ref, Role::Reference));
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
      Range loc = cursor.get_spell();

      IndexFuncId called_id = db->ToFuncId(HashUsr(ref->referencedEntity->USR));
      IndexFunc* called = db->Resolve(called_id);

      std::string_view short_name = called->def.ShortName();
      // libclang doesn't provide a nice api to check if the given function
      // call is implicit. ref->kind should probably work (it's either direct
      // or implicit), but libclang only supports implicit for objective-c.
      bool is_implicit =
          CanBeCalledImplicitly(ref->referencedEntity->kind) &&
          // Treats empty short_name as an implicit call like implicit move
          // constructor in `vector<int> a = f();`
          (short_name.empty() ||
           // For explicit destructor call, ref->cursor may be "~" while
           // called->def.short_name is "~A"
           // "~A" is not a substring of ref->cursor, but we should take this
           // case as not `is_implicit`.
           (short_name[0] != '~' &&
            !CursorSpellingContainsString(ref->cursor, param->tu->cx_tu,
                                          short_name)));

      // Extents have larger ranges and thus less specific, and will be
      // overriden by other functions if exist.
      //
      // Type-dependent member access expressions do not have useful spelling
      // ranges. See the comment above for the CXIdxEntity_Field case.
      if (is_implicit)
        loc = cursor.get_extent();
      else
        CheckTypeDependentMemberRefExpr(&loc, cursor, param, db);

      OnIndexReference_Function(
          db, loc, ref->container->cursor, called_id,
          GetRole(ref, Role::Call) |
              (is_implicit ? Role::Implicit : Role::None));

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
      if (param->config->index.attributeMakeCallsToCtor && is_template &&
          str_begin("make", ref->referencedEntity->name)) {
        // Try to find the return type of called function. That type will have
        // the constructor function we add a usage to.
        optional<ClangCursor> opt_found_type = FindType(ref->cursor);
        if (opt_found_type) {
          Usr ctor_type_usr = opt_found_type->get_referenced().get_usr_hash();
          ClangCursor call_cursor = ref->cursor;

          // Build a type description from the parameters of the call, so we
          // can try to find a constructor with the same type description.
          std::vector<std::string> call_type_desc;
          for (ClangType type : call_cursor.get_type().get_arguments()) {
            std::string type_desc = type.get_spell_name();
            if (!type_desc.empty())
              call_type_desc.push_back(type_desc);
          }

          // Try to find the constructor and add a reference.
          optional<Usr> ctor_usr =
              param->ctors.TryFindConstructorUsr(ctor_type_usr, call_type_desc);
          if (ctor_usr) {
            IndexFunc* ctor = db->Resolve(db->ToFuncId(*ctor_usr));
            ctor->uses.push_back(Use(loc, Id<void>(), SymbolKind::File,
                                     Role::Call | Role::Implicit, {}));
          }
        }
      }

      break;
    }

    case CXIdxEntity_ObjCCategory:
    case CXIdxEntity_ObjCProtocol:
    case CXIdxEntity_ObjCClass:
    case CXIdxEntity_Typedef:
    case CXIdxEntity_CXXInterface:  // MSVC __interface
    case CXIdxEntity_CXXTypeAlias:
    case CXIdxEntity_Enum:
    case CXIdxEntity_Union:
    case CXIdxEntity_Struct:
    case CXIdxEntity_CXXClass: {
      referenced = referenced.template_specialization_to_template_definition();
      IndexType* ref_type =
          db->Resolve(db->ToTypeId(referenced.get_usr_hash()));
      if (!ref->parentEntity || IsDeclContext(ref->parentEntity->kind))
        AddUseSpell(db, ref_type->declarations, ref->cursor);
      else
        AddUseSpell(db, ref_type->uses, ref->cursor);
      break;
    }
  }
}

optional<std::vector<std::unique_ptr<IndexFile>>> Parse(
    Config* config,
    FileConsumerSharedState* file_consumer_shared,
    std::string file,
    const std::vector<std::string>& args,
    const std::vector<FileContents>& file_contents,
    PerformanceImportFile* perf,
    ClangIndex* index,
    bool dump_ast) {
  if (!config->index.enabled)
    return nullopt;

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
    return nullopt;

  perf->index_parse = timer.ElapsedMicrosecondsAndReset();

  if (dump_ast)
    Dump(clang_getTranslationUnitCursor(tu->cx_tu));

  return ParseWithTu(config, file_consumer_shared, perf, tu.get(), index, file,
                     args, unsaved_files);
}

optional<std::vector<std::unique_ptr<IndexFile>>> ParseWithTu(
    Config* config,
    FileConsumerSharedState* file_consumer_shared,
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
  IndexParam param(config, tu, &file_consumer);
  for (const CXUnsavedFile& contents : file_contents) {
    param.file_contents[contents.Filename] = FileContents(
        contents.Filename, std::string(contents.Contents, contents.Length));
  }

  CXFile cx_file = clang_getFile(tu->cx_tu, file.c_str());
  param.primary_file = ConsumeFile(&param, cx_file);

  CXIndexAction index_action = clang_IndexAction_create(index->cx_index);

  // |index_result| is a CXErrorCode instance.
  int index_result = clang_indexTranslationUnit(
      index_action, &param, &callback, sizeof(IndexerCallbacks),
      CXIndexOpt_IndexFunctionLocalSymbols |
          CXIndexOpt_SkipParsedBodiesInSession |
          CXIndexOpt_IndexImplicitTemplateInstantiations,
      tu->cx_tu);
  if (index_result != CXError_Success) {
    LOG_S(ERROR) << "Indexing " << file
                 << " failed with errno=" << index_result;
    return nullopt;
  }

  clang_IndexAction_dispose(index_action);

  ClangCursor(clang_getTranslationUnitCursor(tu->cx_tu))
      .VisitChildren(&VisitMacroDefinitionAndExpansions, &param);

  perf->index_build = timer.ElapsedMicrosecondsAndReset();

  std::unordered_map<std::string, int> inc_to_line;
  // TODO
  if (param.primary_file)
    for (auto& inc : param.primary_file->includes)
      inc_to_line[inc.resolved_path] = inc.line;

  auto result = param.file_consumer->TakeLocalState();
  for (std::unique_ptr<IndexFile>& entry : result) {
    entry->import_file = file;
    entry->args = args;
    for (IndexFunc& func : entry->funcs) {
      // e.g. declaration + out-of-line definition
      Uniquify(func.derived);
      Uniquify(func.uses);
    }
    for (IndexType& type : entry->types) {
      Uniquify(type.derived);
      Uniquify(type.uses);
      // e.g. declaration + out-of-line definition
      Uniquify(type.def.funcs);
    }
    for (IndexVar& var : entry->vars)
      Uniquify(var.uses);

    if (param.primary_file) {
      // If there are errors, show at least one at the include position.
      auto it = inc_to_line.find(entry->path);
      if (it != inc_to_line.end()) {
        int line = it->second;
        for (auto ls_diagnostic : entry->diagnostics_) {
          if (ls_diagnostic.severity != lsDiagnosticSeverity::Error)
            continue;
          ls_diagnostic.range =
              lsRange(lsPosition(line, 10), lsPosition(line, 10));
          param.primary_file->diagnostics_.push_back(ls_diagnostic);
          break;
        }
      }
    }

    // Update file contents and modification time.
    entry->last_modification_time = param.file_modification_times[entry->path];

    // Update dependencies for the file. Do not include the file in its own
    // dependency set.
    entry->dependencies = param.seen_files;
    entry->dependencies.erase(
        std::remove(entry->dependencies.begin(), entry->dependencies.end(),
                    entry->path),
        entry->dependencies.end());
  }

  return std::move(result);
}

void ConcatTypeAndName(std::string& type, const std::string& name) {
  if (type.size() &&
      (type.back() != ' ' && type.back() != '*' && type.back() != '&'))
    type.push_back(' ');
  type.append(name);
}

void IndexInit() {
  clang_enableStackTraces();
  if (!getenv("LIBCLANG_DISABLE_CRASH_RECOVERY"))
    clang_toggleCrashRecovery(1);
}

void ClangSanityCheck() {
  std::vector<const char*> args = {"clang", "index_tests/vars/class_member.cc"};
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
  callback.ppIncludedFile =
      [](CXClientData client_data,
         const CXIdxIncludedFileInfo* file) -> CXIdxClientFile {
    return nullptr;
  };
  callback.importedASTFile =
      [](CXClientData client_data,
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

// |SymbolRef| is serialized this way.
// |Use| also uses this though it has an extra field |file|,
// which is not used by Index* so it does not need to be serialized.
void Reflect(Reader& visitor, Reference& value) {
  if (visitor.Format() == SerializeFormat::Json) {
    std::string t = visitor.GetString();
    char* s = const_cast<char*>(t.c_str());
    value.range = Range(s);
    s = strchr(s, '|');
    value.id.id = RawId(strtol(s + 1, &s, 10));
    value.kind = static_cast<SymbolKind>(strtol(s + 1, &s, 10));
    value.role = static_cast<Role>(strtol(s + 1, &s, 10));
  } else {
    Reflect(visitor, value.range);
    Reflect(visitor, value.id);
    Reflect(visitor, value.kind);
    Reflect(visitor, value.role);
  }
}
void Reflect(Writer& visitor, Reference& value) {
  if (visitor.Format() == SerializeFormat::Json) {
    std::string s = value.range.ToString();
    // RawId(-1) -> "-1"
    s += '|' + std::to_string(
                   static_cast<std::make_signed<RawId>::type>(value.id.id));
    s += '|' + std::to_string(int(value.kind));
    s += '|' + std::to_string(int(value.role));
    Reflect(visitor, s);
  } else {
    Reflect(visitor, value.range);
    Reflect(visitor, value.id);
    Reflect(visitor, value.kind);
    Reflect(visitor, value.role);
  }
}
