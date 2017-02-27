#include "indexer.h"

#include "serializer.h"

IndexedFile::IndexedFile(const std::string& path, IdCache* id_cache)
  : path(path), id_cache(id_cache) {

  // TODO: Reconsider if we should still be reusing the same id_cache.
  // Preallocate any existing resolved ids.
  for (const auto& entry : id_cache->usr_to_type_id)
    types.push_back(IndexedTypeDef(entry.second, entry.first));
  for (const auto& entry : id_cache->usr_to_func_id)
    funcs.push_back(IndexedFuncDef(entry.second, entry.first));
  for (const auto& entry : id_cache->usr_to_var_id)
    vars.push_back(IndexedVarDef(entry.second, entry.first));
}

// TODO: Optimize for const char*?
TypeId IndexedFile::ToTypeId(const std::string& usr) {
  auto it = id_cache->usr_to_type_id.find(usr);
  if (it != id_cache->usr_to_type_id.end())
    return it->second;

  TypeId id(types.size());
  types.push_back(IndexedTypeDef(id, usr));
  id_cache->usr_to_type_id[usr] = id;
  id_cache->type_id_to_usr[id] = usr;
  return id;
}
FuncId IndexedFile::ToFuncId(const std::string& usr) {
  auto it = id_cache->usr_to_func_id.find(usr);
  if (it != id_cache->usr_to_func_id.end())
    return it->second;

  FuncId id(funcs.size());
  funcs.push_back(IndexedFuncDef(id, usr));
  id_cache->usr_to_func_id[usr] = id;
  id_cache->func_id_to_usr[id] = usr;
  return id;
}
VarId IndexedFile::ToVarId(const std::string& usr) {
  auto it = id_cache->usr_to_var_id.find(usr);
  if (it != id_cache->usr_to_var_id.end())
    return it->second;

  VarId id(vars.size());
  vars.push_back(IndexedVarDef(id, usr));
  id_cache->usr_to_var_id[usr] = id;
  id_cache->var_id_to_usr[id] = usr;
  return id;
}

TypeId IndexedFile::ToTypeId(const CXCursor& cursor) {
  return ToTypeId(clang::Cursor(cursor).get_usr());
}

FuncId IndexedFile::ToFuncId(const CXCursor& cursor) {
  return ToFuncId(clang::Cursor(cursor).get_usr());
}

VarId IndexedFile::ToVarId(const CXCursor& cursor) {
  return ToVarId(clang::Cursor(cursor).get_usr());
}


IndexedTypeDef* IndexedFile::Resolve(TypeId id) {
  return &types[id.id];
}
IndexedFuncDef* IndexedFile::Resolve(FuncId id) {
  return &funcs[id.id];
}
IndexedVarDef* IndexedFile::Resolve(VarId id) {
  return &vars[id.id];
}

std::string IndexedFile::ToString() {
  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  writer.SetFormatOptions(
    rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  Serialize(writer, this);

  return output.GetString();
}

IndexedTypeDef::IndexedTypeDef(TypeId id, const std::string& usr) : id(id), def(usr) {
  assert(usr.size() > 0);
  //std::cout << "Creating type with usr " << usr << std::endl;
}

void IndexedTypeDef::AddUsage(Location loc, bool insert_if_not_present) {
  for (int i = uses.size() - 1; i >= 0; --i) {
    if (uses[i].IsEqualTo(loc)) {
      if (loc.interesting)
        uses[i].interesting = true;
      return;
    }
  }

  if (insert_if_not_present)
    uses.push_back(loc);
}


template<typename T>
bool Contains(const std::vector<T>& vec, const T& element) {
  for (const T& entry : vec) {
    if (entry == element)
      return true;
  }
  return false;
}














int abortQuery(CXClientData client_data, void *reserved) {
  // 0 -> continue
  return 0;
}
void diagnostic(CXClientData client_data, CXDiagnosticSet, void *reserved) {}

CXIdxClientFile enteredMainFile(CXClientData client_data, CXFile mainFile, void *reserved) {
  return nullptr;
}

CXIdxClientFile ppIncludedFile(CXClientData client_data, const CXIdxIncludedFileInfo *) {
  return nullptr;
}

CXIdxClientASTFile importedASTFile(CXClientData client_data, const CXIdxImportedASTFileInfo *) {
  return nullptr;
}

CXIdxClientContainer startedTranslationUnit(CXClientData client_data, void *reserved) {
  return nullptr;
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





struct FindChildOfKindParam {
  CXCursorKind target_kind;
  optional<clang::Cursor> result;

  FindChildOfKindParam(CXCursorKind target_kind) : target_kind(target_kind) {}
};

clang::VisiterResult FindChildOfKindVisitor(clang::Cursor cursor, clang::Cursor parent, FindChildOfKindParam* param) {
  if (cursor.get_kind() == param->target_kind) {
    param->result = cursor;
    return clang::VisiterResult::Break;
  }

  return clang::VisiterResult::Recurse;
}

optional<clang::Cursor> FindChildOfKind(clang::Cursor cursor, CXCursorKind kind) {
  FindChildOfKindParam param(kind);
  cursor.VisitChildren(&FindChildOfKindVisitor, &param);
  return param.result;
}





clang::VisiterResult FindTypeVisitor(clang::Cursor cursor, clang::Cursor parent, optional<clang::Cursor>* result) {
  switch (cursor.get_kind()) {
  case CXCursor_TypeRef:
  case CXCursor_TemplateRef:
    *result = cursor;
    return clang::VisiterResult::Break;
  }

  return clang::VisiterResult::Recurse;
}

optional<clang::Cursor> FindType(clang::Cursor cursor) {
  optional<clang::Cursor> result;
  cursor.VisitChildren(&FindTypeVisitor, &result);
  return result;
}







struct NamespaceHelper {
  std::unordered_map<std::string, std::string> container_usr_to_qualified_name;

  void RegisterQualifiedName(std::string usr, const CXIdxContainerInfo* container, std::string qualified_name) {
    if (container) {
      std::string container_usr = clang::Cursor(container->cursor).get_usr();
      auto it = container_usr_to_qualified_name.find(container_usr);
      if (it != container_usr_to_qualified_name.end()) {
        container_usr_to_qualified_name[usr] = it->second + qualified_name + "::";
        return;
      }
    }

    container_usr_to_qualified_name[usr] = qualified_name + "::";
  }

  std::string QualifiedName(const CXIdxContainerInfo* container, std::string unqualified_name) {
    if (container) {
      std::string container_usr = clang::Cursor(container->cursor).get_usr();
      auto it = container_usr_to_qualified_name.find(container_usr);
      if (it != container_usr_to_qualified_name.end())
        return it->second + unqualified_name;

      // Anonymous namespaces are not processed by indexDeclaration. If we
      // encounter one insert it into map.
      if (container->cursor.kind == CXCursor_Namespace) {
        assert(clang::Cursor(container->cursor).get_spelling() == "");
        container_usr_to_qualified_name[container_usr] = "::";
        return "::" + unqualified_name;
      }
    }
    return unqualified_name;
  }
};

struct IndexParam {
  IndexedFile* db;
  NamespaceHelper* ns;

  // Record the last type usage location we recorded. Clang will sometimes
  // visit the same expression twice so we wan't to avoid double-reporting
  // usage information for those locations.
  Location last_type_usage_location;
  Location last_func_usage_location;

  IndexParam(IndexedFile* db, NamespaceHelper* ns) : db(db), ns(ns) {}
};

/*
std::string GetNamespacePrefx(const CXIdxDeclInfo* decl) {
  const CXIdxContainerInfo* container = decl->lexicalContainer;
  while (container) {

  }
}
*/

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
  IndexedFile* db;
  bool is_interesting;
  int has_processed_any = false;
  optional<clang::Cursor> previous_cursor;
  optional<TypeId> initial_type;

  VisitDeclForTypeUsageParam(IndexedFile* db, bool is_interesting)
    : db(db), is_interesting(is_interesting) {}
};

void VisitDeclForTypeUsageVisitorHandler(clang::Cursor cursor, VisitDeclForTypeUsageParam* param) {
  param->has_processed_any = true;
  IndexedFile* db = param->db;

  // TODO: Something in STL (type_traits)? reports an empty USR.
  std::string referenced_usr = cursor.get_referenced().get_usr();
  if (referenced_usr == "")
    return;

  TypeId ref_type_id = db->ToTypeId(referenced_usr);
  if (!param->initial_type)
    param->initial_type = ref_type_id;

  if (param->is_interesting) {
    IndexedTypeDef* ref_type_def = db->Resolve(ref_type_id);
    Location loc = db->id_cache->Resolve(cursor, true /*interesting*/);
    ref_type_def->AddUsage(loc);
  }
}

clang::VisiterResult VisitDeclForTypeUsageVisitor(clang::Cursor cursor, clang::Cursor parent, VisitDeclForTypeUsageParam* param) {
  switch (cursor.get_kind()) {
  case CXCursor_TemplateRef:
  case CXCursor_TypeRef:
    if (param->previous_cursor) {
      VisitDeclForTypeUsageVisitorHandler(param->previous_cursor.value(), param);

      // This if is inside the above if because if there are multiple TypeRefs,
      // we always want to process the first one. If we did not always process
      // the first one, we cannot tell if there are more TypeRefs after it and
      // logic for fetching the return type breaks. This happens in ParmDecl
      // instances which only have one TypeRef child but are not interesting
      // usages.
      if (!param->is_interesting)
        return clang::VisiterResult::Break;
    }

    param->previous_cursor = cursor;
  }

  return clang::VisiterResult::Continue;
}

optional<TypeId> ResolveDeclToType(IndexedFile* db, clang::Cursor decl_cursor,
  bool is_interesting, const CXIdxContainerInfo* semantic_container,
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
  if (IsTypeDefinition(semantic_container) && !IsTypeDefinition(lexical_container)) {
    assert(decl_cursor.is_definition());
    process_last_type_ref = false;
  }

  VisitDeclForTypeUsageParam param(db, is_interesting);
  decl_cursor.VisitChildren(&VisitDeclForTypeUsageVisitor, &param);

  // VisitDeclForTypeUsageVisitor guarantees that if there are multiple TypeRef
  // children, the first one will always be visited.
  if (param.previous_cursor && process_last_type_ref) {
    VisitDeclForTypeUsageVisitorHandler(param.previous_cursor.value(), &param);
  }
  else {
    // If we are not processing the last type ref, it *must* be a TypeRef (ie,
    // and not a TemplateRef).
    assert(!param.previous_cursor.has_value() ||
      param.previous_cursor.value().get_kind() == CXCursor_TypeRef);
  }

  return param.initial_type;
}


void indexDeclaration(CXClientData client_data, const CXIdxDeclInfo* decl) {
  bool is_system_def = clang_Location_isInSystemHeader(clang_getCursorLocation(decl->cursor));

  IndexParam* param = static_cast<IndexParam*>(client_data);
  IndexedFile* db = param->db;
  NamespaceHelper* ns = param->ns;

  switch (decl->entityInfo->kind) {
  case CXIdxEntity_CXXNamespace:
  {
    ns->RegisterQualifiedName(decl->entityInfo->USR, decl->semanticContainer, decl->entityInfo->name);
    break;
  }

  case CXIdxEntity_EnumConstant:
  case CXIdxEntity_Field:
  case CXIdxEntity_Variable:
  case CXIdxEntity_CXXStaticVariable:
  {
    clang::Cursor decl_cursor = decl->cursor;
    VarId var_id = db->ToVarId(decl->entityInfo->USR);
    IndexedVarDef* var_def = db->Resolve(var_id);

    var_def->is_bad_def = is_system_def;

    // TODO: Eventually run with this if. Right now I want to iron out bugs this may shadow.
    // TODO: Verify this gets called multiple times
    //if (!decl->isRedeclaration) {
    var_def->def.short_name = decl->entityInfo->name;
    var_def->def.qualified_name = ns->QualifiedName(decl->semanticContainer, var_def->def.short_name);
    //}

    Location decl_loc = db->id_cache->Resolve(decl->loc, false /*interesting*/);
    if (decl->isDefinition)
      var_def->def.definition = decl_loc;
    else
      var_def->def.declaration = decl_loc;
    var_def->uses.push_back(decl_loc);


    // Declaring variable type information. Note that we do not insert an
    // interesting reference for parameter declarations - that is handled when
    // the function declaration is encountered since we won't receive ParmDecl
    // declarations for unnamed parameters.
    optional<TypeId> var_type = ResolveDeclToType(db, decl_cursor, decl_cursor.get_kind() != CXCursor_ParmDecl /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);
    if (var_type.has_value())
      var_def->def.variable_type = var_type.value();


    if (decl->isDefinition && IsTypeDefinition(decl->semanticContainer)) {
      TypeId declaring_type_id = db->ToTypeId(decl->semanticContainer->cursor);
      IndexedTypeDef* declaring_type_def = db->Resolve(declaring_type_id);
      var_def->def.declaring_type = declaring_type_id;
      declaring_type_def->def.vars.push_back(var_id);
    }

    break;
  }

  case CXIdxEntity_Function:
  case CXIdxEntity_CXXConstructor:
  case CXIdxEntity_CXXDestructor:
  case CXIdxEntity_CXXInstanceMethod:
  case CXIdxEntity_CXXStaticMethod:
  case CXIdxEntity_CXXConversionFunction:
  {
    clang::Cursor decl_cursor = decl->cursor;
    FuncId func_id = db->ToFuncId(decl->entityInfo->USR);
    IndexedFuncDef* func_def = db->Resolve(func_id);

    func_def->is_bad_def = is_system_def;

    // TODO: Eventually run with this if. Right now I want to iron out bugs this may shadow.
    //if (!decl->isRedeclaration) {
    func_def->def.short_name = decl->entityInfo->name;
    func_def->def.qualified_name = ns->QualifiedName(decl->semanticContainer, func_def->def.short_name);
    //}

    Location decl_loc = db->id_cache->Resolve(decl->loc, false /*interesting*/);
    if (decl->isDefinition)
      func_def->def.definition = decl_loc;
    else
      func_def->declarations.push_back(decl_loc);
    func_def->uses.push_back(decl_loc);

    bool is_pure_virtual = clang_CXXMethod_isPureVirtual(decl->cursor);
    bool is_ctor_or_dtor = decl->entityInfo->kind == CXIdxEntity_CXXConstructor || decl->entityInfo->kind == CXIdxEntity_CXXDestructor;
    //bool process_declaring_type = is_pure_virtual || is_ctor_or_dtor;

    // Add function usage information. We only want to do it once per
    // definition/declaration. Do it on definition since there should only ever
    // be one of those in the entire program.
    if (IsTypeDefinition(decl->semanticContainer)) {
      TypeId declaring_type_id = db->ToTypeId(decl->semanticContainer->cursor);
      IndexedTypeDef* declaring_type_def = db->Resolve(declaring_type_id);
      func_def->def.declaring_type = declaring_type_id;

      // Mark a type reference at the ctor/dtor location.
      // TODO: Should it be interesting?
      if (is_ctor_or_dtor) {
        Location type_usage_loc = decl_loc;
        declaring_type_def->AddUsage(type_usage_loc);
      }

      // Register function in declaring type if it hasn't been registered yet.
      if (!Contains(declaring_type_def->def.funcs, func_id))
        declaring_type_def->def.funcs.push_back(func_id);
    }



    // We don't actually need to know the return type, but we need to mark it
    // as an interesting usage.
    ResolveDeclToType(db, decl_cursor, true /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);

    //TypeResolution ret_type = ResolveToType(db, decl_cursor.get_type().get_return_type());
    //if (ret_type.resolved_type)
    //  AddInterestingUsageToType(db, ret_type, FindLocationOfTypeSpecifier(decl_cursor));

    if (decl->isDefinition || is_pure_virtual) {
      // Mark type usage for parameters as interesting. We handle this here
      // instead of inside var declaration because clang will not emit a var
      // declaration for an unnamed parameter, but we still want to mark the
      // usage as interesting.
      // TODO: Do a similar thing for function decl parameter usages. Mark
      //       prototype params as interesting type usages but also relate mark
      //       them as as usages on the primary variable - requires USR to be
      //       the same. We can work around it by declaring which variables a
      //       parameter has declared and update the USR in the definition.
      clang::Cursor cursor = decl->cursor;
      for (clang::Cursor arg : cursor.get_arguments()) {
        switch (arg.get_kind()) {
        case CXCursor_ParmDecl:
          // We don't need to know the arg type, but we do want to mark it as
          // an interesting usage.
          ResolveDeclToType(db, arg, true /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);

          //TypeResolution arg_type = ResolveToType(db, arg.get_type());
          //if (arg_type.resolved_type)
          //  AddInterestingUsageToType(db, arg_type, FindLocationOfTypeSpecifier(arg));
          break;
        }
      }


      // Process inheritance.
      //void clang_getOverriddenCursors(CXCursor cursor, CXCursor **overridden, unsigned *num_overridden);
      //void clang_disposeOverriddenCursors(CXCursor *overridden);
      if (clang_CXXMethod_isVirtual(decl->cursor)) {
        CXCursor* overridden;
        unsigned int num_overridden;
        clang_getOverriddenCursors(decl->cursor, &overridden, &num_overridden);

        // TODO: How to handle multiple parent overrides??
        for (unsigned int i = 0; i < num_overridden; ++i) {
          clang::Cursor parent = overridden[i];
          FuncId parent_id = db->ToFuncId(parent.get_usr());
          IndexedFuncDef* parent_def = db->Resolve(parent_id);
          func_def = db->Resolve(func_id); // ToFuncId invalidated func_def

          func_def->def.base = parent_id;
          parent_def->derived.push_back(func_id);
        }

        clang_disposeOverriddenCursors(overridden);
      }
    }

    /*
    optional<FuncId> base;
    std::vector<FuncId> derived;
    std::vector<VarId> locals;
    std::vector<FuncRef> callers;
    std::vector<FuncRef> callees;
    std::vector<Location> uses;
    */
    break;
  }

  case CXIdxEntity_Typedef:
  case CXIdxEntity_CXXTypeAlias:
  {
    optional<TypeId> alias_of = ResolveDeclToType(db, decl->cursor, true /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);

    TypeId type_id = db->ToTypeId(decl->entityInfo->USR);
    IndexedTypeDef* type_def = db->Resolve(type_id);

    type_def->is_bad_def = is_system_def;

    if (alias_of)
      type_def->def.alias_of = alias_of.value();

    type_def->def.short_name = decl->entityInfo->name;
    type_def->def.qualified_name = ns->QualifiedName(decl->semanticContainer, type_def->def.short_name);

    Location decl_loc = db->id_cache->Resolve(decl->loc, true /*interesting*/);
    type_def->def.definition = decl_loc.WithInteresting(false);
    type_def->AddUsage(decl_loc);
    break;
  }

  case CXIdxEntity_Enum:
  case CXIdxEntity_Union:
  case CXIdxEntity_Struct:
  case CXIdxEntity_CXXClass:
  {
    TypeId type_id = db->ToTypeId(decl->entityInfo->USR);
    IndexedTypeDef* type_def = db->Resolve(type_id);

    type_def->is_bad_def = is_system_def;

    // TODO: Eventually run with this if. Right now I want to iron out bugs this may shadow.
    // TODO: For type section, verify if this ever runs for non definitions?
    //if (!decl->isRedeclaration) {

    // name can be null in an anonymous struct (see tests/types/anonymous_struct.cc).
    if (decl->entityInfo->name) {
      ns->RegisterQualifiedName(decl->entityInfo->USR, decl->semanticContainer, decl->entityInfo->name);
      type_def->def.short_name = decl->entityInfo->name;
    }
    else {
      type_def->def.short_name = "<anonymous>";
    }

    type_def->def.qualified_name = ns->QualifiedName(decl->semanticContainer, type_def->def.short_name);

    // }

    assert(decl->isDefinition);
    Location decl_loc = db->id_cache->Resolve(decl->loc, true /*interesting*/);
    type_def->def.definition = decl_loc.WithInteresting(false);
    type_def->AddUsage(decl_loc);

    //type_def->alias_of
    //type_def->funcs
    //type_def->types
    //type_def->uses
    //type_def->vars

    // Add type-level inheritance information.
    CXIdxCXXClassDeclInfo const* class_info = clang_index_getCXXClassDeclInfo(decl);
    if (class_info) {
      for (unsigned int i = 0; i < class_info->numBases; ++i) {
        const CXIdxBaseClassInfo* base_class = class_info->bases[i];

        optional<TypeId> parent_type_id = ResolveDeclToType(db, base_class->cursor, true /*is_interesting*/, decl->semanticContainer, decl->lexicalContainer);
        IndexedTypeDef* type_def = db->Resolve(type_id); // type_def ptr could be invalidated by ResolveDeclToType.
        if (parent_type_id) {
          IndexedTypeDef* parent_type_def = db->Resolve(parent_type_id.value());
          parent_type_def->derived.push_back(type_id);
          type_def->def.parents.push_back(parent_type_id.value());
        }
      }
    }
    break;
  }

  default:
    std::cout << "!! Unhandled indexDeclaration:     " << clang::Cursor(decl->cursor).ToString() << " at " << db->id_cache->Resolve(decl->loc, false /*interesting*/).ToString() << std::endl;
    std::cout << "     entityInfo->kind  = " << decl->entityInfo->kind << std::endl;
    std::cout << "     entityInfo->USR   = " << decl->entityInfo->USR << std::endl;
    if (decl->declAsContainer)
      std::cout << "     declAsContainer   = " << clang::Cursor(decl->declAsContainer->cursor).ToString() << std::endl;
    if (decl->semanticContainer)
      std::cout << "     semanticContainer = " << clang::Cursor(decl->semanticContainer->cursor).ToString() << std::endl;
    if (decl->lexicalContainer)
      std::cout << "     lexicalContainer  = " << clang::Cursor(decl->lexicalContainer->cursor).get_usr() << std::endl;
    break;
  }
}

bool IsFunction(CXCursorKind kind) {
  switch (kind) {
  case CXCursor_CXXMethod:
  case CXCursor_FunctionDecl:
    return true;
  }

  return false;
}

void indexEntityReference(CXClientData client_data, const CXIdxEntityRefInfo* ref) {
  IndexParam* param = static_cast<IndexParam*>(client_data);
  IndexedFile* db = param->db;
  clang::Cursor cursor(ref->cursor);

  switch (ref->referencedEntity->kind) {
  case CXIdxEntity_EnumConstant:
  case CXIdxEntity_CXXStaticVariable:
  case CXIdxEntity_Variable:
  case CXIdxEntity_Field:
  {
    VarId var_id = db->ToVarId(ref->referencedEntity->cursor);
    IndexedVarDef* var_def = db->Resolve(var_id);
    var_def->uses.push_back(db->id_cache->Resolve(ref->loc, false /*interesting*/));
    break;
  }

  case CXIdxEntity_CXXConversionFunction:
  case CXIdxEntity_CXXStaticMethod:
  case CXIdxEntity_CXXInstanceMethod:
  case CXIdxEntity_Function:
  case CXIdxEntity_CXXConstructor:
  case CXIdxEntity_CXXDestructor:
  {
    // TODO: Redirect container to constructor for the following example, ie,
    //       we should be inserting an outgoing function call from the Foo
    //       ctor.
    //
    //  int Gen() { return 5; }
    //  class Foo {
    //    int x = Gen();
    //  }

    // Don't report duplicate usages.
    // TODO: search full history?
    Location loc = db->id_cache->Resolve(ref->loc, false /*interesting*/);
    if (param->last_func_usage_location == loc) break;
    param->last_func_usage_location = loc;

    // Note: be careful, calling db->ToFuncId invalidates the FuncDef* ptrs.
    FuncId called_id = db->ToFuncId(ref->referencedEntity->USR);
    if (IsFunction(ref->container->cursor.kind)) {
      FuncId caller_id = db->ToFuncId(ref->container->cursor);
      IndexedFuncDef* caller_def = db->Resolve(caller_id);
      IndexedFuncDef* called_def = db->Resolve(called_id);

      caller_def->def.callees.push_back(FuncRef(called_id, loc));
      called_def->callers.push_back(FuncRef(caller_id, loc));
      called_def->uses.push_back(loc);
    }
    else {
      IndexedFuncDef* called_def = db->Resolve(called_id);
      called_def->uses.push_back(loc);
    }

    // For constructor/destructor, also add a usage against the type. Clang
    // will insert and visit implicit constructor references, so we also check
    // the location of the ctor call compared to the parent call. If they are
    // the same, this is most likely an implicit ctors.
    clang::Cursor ref_cursor = ref->cursor;
    if (ref->referencedEntity->kind == CXIdxEntity_CXXConstructor ||
      ref->referencedEntity->kind == CXIdxEntity_CXXDestructor) {

      Location parent_loc = db->id_cache->Resolve(ref->parentEntity->cursor, true /*interesting*/);
      Location our_loc = db->id_cache->Resolve(ref->loc, true /*is_interesting*/);
      if (!parent_loc.IsEqualTo(our_loc)) {
        IndexedFuncDef* called_def = db->Resolve(called_id);
        assert(called_def->def.declaring_type.has_value());
        IndexedTypeDef* type_def = db->Resolve(called_def->def.declaring_type.value());
        type_def->AddUsage(our_loc);
      }
    }
    break;
  }

  case CXIdxEntity_Typedef:
  case CXIdxEntity_CXXTypeAlias:
  case CXIdxEntity_Enum:
  case CXIdxEntity_Union:
  case CXIdxEntity_Struct:
  case CXIdxEntity_CXXClass:
  {
    TypeId referenced_id = db->ToTypeId(ref->referencedEntity->USR);
    IndexedTypeDef* referenced_def = db->Resolve(referenced_id);

    // We will not get a declaration visit for forward declared types. Try to mark them as non-bad 
    // defs here so we will output usages/etc.
    if (referenced_def->is_bad_def) {
      bool is_system_def = clang_Location_isInSystemHeader(clang_getCursorLocation(ref->referencedEntity->cursor));
      Location loc = db->id_cache->Resolve(ref->referencedEntity->cursor, false /*interesting*/);
      if (!is_system_def && loc.raw_file_id != -1)
        referenced_def->is_bad_def = false;
    }
    //
    // The following will generate two TypeRefs to Foo, both located at the
    // same spot (line 3, column 3). One of the parents will be set to
    // CXIdxEntity_Variable, the other will be CXIdxEntity_Function. There does
    // not appear to be a good way to disambiguate these references, as using
    // parent type alone breaks other indexing tasks.
    //
    // To work around this, we check to see if the usage location has been
    // inserted into all_uses previously.
    //
    //  struct Foo {};
    //  void Make() {
    //    Foo f;
    //  }
    //
    referenced_def->AddUsage(db->id_cache->Resolve(ref->loc, false /*interesting*/));
    break;
  }

  default:
    std::cout << "!! Unhandled indexEntityReference: " << cursor.ToString() << " at " << db->id_cache->Resolve(ref->loc, false /*interesting*/).ToString() << std::endl;
    std::cout << "     ref->referencedEntity->kind = " << ref->referencedEntity->kind << std::endl;
    if (ref->parentEntity)
      std::cout << "     ref->parentEntity->kind = " << ref->parentEntity->kind << std::endl;
    std::cout << "     ref->loc          = " << db->id_cache->Resolve(ref->loc, false /*interesting*/).ToString() << std::endl;
    std::cout << "     ref->kind         = " << ref->kind << std::endl;
    if (ref->parentEntity)
      std::cout << "     parentEntity      = " << clang::Cursor(ref->parentEntity->cursor).ToString() << std::endl;
    if (ref->referencedEntity)
      std::cout << "     referencedEntity  = " << clang::Cursor(ref->referencedEntity->cursor).ToString() << std::endl;
    if (ref->container)
      std::cout << "     container         = " << clang::Cursor(ref->container->cursor).ToString() << std::endl;
    break;
  }
}


IndexedFile Parse(IdCache* id_cache, std::string filename, std::vector<std::string> args, bool dump_ast) {
  clang::Index index(0 /*excludeDeclarationsFromPCH*/, 0 /*displayDiagnostics*/);
  clang::TranslationUnit tu(index, filename, args);

  if (dump_ast)
    Dump(tu.document_cursor());

  CXIndexAction index_action = clang_IndexAction_create(index.cx_index);

  IndexerCallbacks callbacks[] = {
    { &abortQuery, &diagnostic, &enteredMainFile, &ppIncludedFile, &importedASTFile, &startedTranslationUnit, &indexDeclaration, &indexEntityReference }
    /*
    callbacks.abortQuery = &abortQuery;
    callbacks.diagnostic = &diagnostic;
    callbacks.enteredMainFile = &enteredMainFile;
    callbacks.ppIncludedFile = &ppIncludedFile;
    callbacks.importedASTFile = &importedASTFile;
    callbacks.startedTranslationUnit = &startedTranslationUnit;
    callbacks.indexDeclaration = &indexDeclaration;
    callbacks.indexEntityReference = &indexEntityReference;
    */
  };

  IndexedFile db(filename, id_cache);
  NamespaceHelper ns;
  IndexParam param(&db, &ns);
  clang_indexTranslationUnit(index_action, &param, callbacks, sizeof(callbacks),
    CXIndexOpt_IndexFunctionLocalSymbols | CXIndexOpt_SkipParsedBodiesInSession, tu.cx_tu);

  clang_IndexAction_dispose(index_action);

  return db;
}
