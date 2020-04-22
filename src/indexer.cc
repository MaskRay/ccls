// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "indexer.hh"

#include "clang_tu.hh"
#include "log.hh"
#include "pipeline.hh"
#include "platform.hh"
#include "sema_manager.hh"

#include <clang/AST/AST.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Index/IndexDataConsumer.h>
#include <clang/Index/IndexingAction.h>
#include <clang/Index/USRGeneration.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/Path.h>

#include <algorithm>
#include <inttypes.h>
#include <map>
#include <unordered_set>

using namespace clang;

namespace ccls {
namespace {

GroupMatch *multiVersionMatcher;

struct File {
  std::string path;
  int64_t mtime;
  std::string content;
  std::unique_ptr<IndexFile> db;
};

struct IndexParam {
  std::unordered_map<llvm::sys::fs::UniqueID, File> uid2file;
  std::unordered_map<llvm::sys::fs::UniqueID, bool> uid2multi;
  struct DeclInfo {
    Usr usr;
    std::string short_name;
    std::string qualified;
  };
  std::unordered_map<const Decl *, DeclInfo> decl2Info;

  VFS &vfs;
  ASTContext *ctx;
  bool no_linkage;
  IndexParam(VFS &vfs, bool no_linkage) : vfs(vfs), no_linkage(no_linkage) {}

  void seenFile(const FileEntry &file) {
    // If this is the first time we have seen the file (ignoring if we are
    // generating an index for it):
    auto [it, inserted] = uid2file.try_emplace(file.getUniqueID());
    if (inserted) {
      std::string path = pathFromFileEntry(file);
      it->second.path = path;
      it->second.mtime = file.getModificationTime();
      if (!it->second.mtime)
        if (auto tim = lastWriteTime(path))
          it->second.mtime = *tim;
      if (std::optional<std::string> content = readContent(path))
        it->second.content = *content;

      if (!vfs.stamp(path, it->second.mtime, no_linkage ? 3 : 1))
        return;
      it->second.db =
          std::make_unique<IndexFile>(path, it->second.content, no_linkage);
    }
  }

  IndexFile *consumeFile(const FileEntry &fe) {
    seenFile(fe);
    return uid2file[fe.getUniqueID()].db.get();
  }

  bool useMultiVersion(const FileEntry &fe) {
    auto it = uid2multi.try_emplace(fe.getUniqueID());
    if (it.second)
      it.first->second = multiVersionMatcher->matches(pathFromFileEntry(fe));
    return it.first->second;
  }
};

StringRef getSourceInRange(const SourceManager &sm, const LangOptions &langOpts,
                           SourceRange sr) {
  SourceLocation bloc = sr.getBegin(), eLoc = sr.getEnd();
  std::pair<FileID, unsigned> bInfo = sm.getDecomposedLoc(bloc),
                              eInfo = sm.getDecomposedLoc(eLoc);
  bool invalid = false;
  StringRef buf = sm.getBufferData(bInfo.first, &invalid);
  if (invalid)
    return "";
  return buf.substr(bInfo.second,
                    eInfo.second +
                        Lexer::MeasureTokenLength(eLoc, sm, langOpts) -
                        bInfo.second);
}

Kind getKind(const Decl *d, SymbolKind &kind) {
  switch (d->getKind()) {
  case Decl::LinkageSpec:
    return Kind::Invalid;
  case Decl::Namespace:
  case Decl::NamespaceAlias:
    kind = SymbolKind::Namespace;
    return Kind::Type;
  case Decl::ObjCCategory:
  case Decl::ObjCCategoryImpl:
  case Decl::ObjCImplementation:
  case Decl::ObjCInterface:
  case Decl::ObjCProtocol:
    kind = SymbolKind::Interface;
    return Kind::Type;
  case Decl::ObjCMethod:
    kind = SymbolKind::Method;
    return Kind::Func;
  case Decl::ObjCProperty:
    kind = SymbolKind::Property;
    return Kind::Type;
  case Decl::ClassTemplate:
    kind = SymbolKind::Class;
    return Kind::Type;
  case Decl::FunctionTemplate:
    kind = SymbolKind::Function;
    return Kind::Func;
  case Decl::TypeAliasTemplate:
    kind = SymbolKind::TypeAlias;
    return Kind::Type;
  case Decl::VarTemplate:
    kind = SymbolKind::Variable;
    return Kind::Var;
  case Decl::TemplateTemplateParm:
    kind = SymbolKind::TypeParameter;
    return Kind::Type;
  case Decl::Enum:
    kind = SymbolKind::Enum;
    return Kind::Type;
  case Decl::CXXRecord:
  case Decl::Record:
    kind = SymbolKind::Class;
    // spec has no Union, use Class
    if (auto *rd = dyn_cast<RecordDecl>(d))
      if (rd->getTagKind() == TTK_Struct)
        kind = SymbolKind::Struct;
    return Kind::Type;
  case Decl::ClassTemplateSpecialization:
  case Decl::ClassTemplatePartialSpecialization:
    kind = SymbolKind::Class;
    return Kind::Type;
  case Decl::TemplateTypeParm:
    kind = SymbolKind::TypeParameter;
    return Kind::Type;
  case Decl::TypeAlias:
  case Decl::Typedef:
  case Decl::UnresolvedUsingTypename:
    kind = SymbolKind::TypeAlias;
    return Kind::Type;
  case Decl::Using:
    kind = SymbolKind::Null; // ignored
    return Kind::Invalid;
  case Decl::Binding:
    kind = SymbolKind::Variable;
    return Kind::Var;
  case Decl::Field:
  case Decl::ObjCIvar:
    kind = SymbolKind::Field;
    return Kind::Var;
  case Decl::Function:
    kind = SymbolKind::Function;
    return Kind::Func;
  case Decl::CXXMethod: {
    const auto *md = cast<CXXMethodDecl>(d);
    kind = md->isStatic() ? SymbolKind::StaticMethod : SymbolKind::Method;
    return Kind::Func;
  }
  case Decl::CXXConstructor:
    kind = SymbolKind::Constructor;
    return Kind::Func;
  case Decl::CXXConversion:
  case Decl::CXXDestructor:
    kind = SymbolKind::Method;
    return Kind::Func;
  case Decl::NonTypeTemplateParm:
    // ccls extension
    kind = SymbolKind::Parameter;
    return Kind::Var;
  case Decl::Var:
  case Decl::Decomposition:
    kind = SymbolKind::Variable;
    return Kind::Var;
  case Decl::ImplicitParam:
  case Decl::ParmVar:
    // ccls extension
    kind = SymbolKind::Parameter;
    return Kind::Var;
  case Decl::VarTemplateSpecialization:
  case Decl::VarTemplatePartialSpecialization:
    kind = SymbolKind::Variable;
    return Kind::Var;
  case Decl::EnumConstant:
    kind = SymbolKind::EnumMember;
    return Kind::Var;
  case Decl::UnresolvedUsingValue:
    kind = SymbolKind::Variable;
    return Kind::Var;
  case Decl::TranslationUnit:
    return Kind::Invalid;

  default:
    return Kind::Invalid;
  }
}

LanguageId getDeclLanguage(const Decl *d) {
  switch (d->getKind()) {
  default:
    return LanguageId::C;
  case Decl::ImplicitParam:
  case Decl::ObjCAtDefsField:
  case Decl::ObjCCategory:
  case Decl::ObjCCategoryImpl:
  case Decl::ObjCCompatibleAlias:
  case Decl::ObjCImplementation:
  case Decl::ObjCInterface:
  case Decl::ObjCIvar:
  case Decl::ObjCMethod:
  case Decl::ObjCProperty:
  case Decl::ObjCPropertyImpl:
  case Decl::ObjCProtocol:
  case Decl::ObjCTypeParam:
    return LanguageId::ObjC;
  case Decl::CXXConstructor:
  case Decl::CXXConversion:
  case Decl::CXXDestructor:
  case Decl::CXXMethod:
  case Decl::CXXRecord:
  case Decl::ClassTemplate:
  case Decl::ClassTemplatePartialSpecialization:
  case Decl::ClassTemplateSpecialization:
  case Decl::Friend:
  case Decl::FriendTemplate:
  case Decl::FunctionTemplate:
  case Decl::LinkageSpec:
  case Decl::Namespace:
  case Decl::NamespaceAlias:
  case Decl::NonTypeTemplateParm:
  case Decl::StaticAssert:
  case Decl::TemplateTemplateParm:
  case Decl::TemplateTypeParm:
  case Decl::UnresolvedUsingTypename:
  case Decl::UnresolvedUsingValue:
  case Decl::Using:
  case Decl::UsingDirective:
  case Decl::UsingShadow:
    return LanguageId::Cpp;
  }
}

// clang/lib/AST/DeclPrinter.cpp
QualType getBaseType(QualType t, bool deduce_auto) {
  QualType baseType = t;
  while (!baseType.isNull() && !baseType->isSpecifierType()) {
    if (const PointerType *pTy = baseType->getAs<PointerType>())
      baseType = pTy->getPointeeType();
    else if (const BlockPointerType *bPy = baseType->getAs<BlockPointerType>())
      baseType = bPy->getPointeeType();
    else if (const ArrayType *aTy = dyn_cast<ArrayType>(baseType))
      baseType = aTy->getElementType();
    else if (const VectorType *vTy = baseType->getAs<VectorType>())
      baseType = vTy->getElementType();
    else if (const ReferenceType *rTy = baseType->getAs<ReferenceType>())
      baseType = rTy->getPointeeType();
    else if (const ParenType *pTy = baseType->getAs<ParenType>())
      baseType = pTy->desugar();
    else if (deduce_auto) {
      if (const AutoType *aTy = baseType->getAs<AutoType>())
        baseType = aTy->getDeducedType();
      else
        break;
    } else
      break;
  }
  return baseType;
}

const Decl *getTypeDecl(QualType t, bool *specialization = nullptr) {
  Decl *d = nullptr;
  t = getBaseType(t.getUnqualifiedType(), true);
  const Type *tp = t.getTypePtrOrNull();
  if (!tp)
    return nullptr;

try_again:
  switch (tp->getTypeClass()) {
  case Type::Typedef:
    d = cast<TypedefType>(tp)->getDecl();
    break;
  case Type::ObjCObject:
    d = cast<ObjCObjectType>(tp)->getInterface();
    break;
  case Type::ObjCInterface:
    d = cast<ObjCInterfaceType>(tp)->getDecl();
    break;
  case Type::Record:
  case Type::Enum:
    d = cast<TagType>(tp)->getDecl();
    break;
  case Type::TemplateTypeParm:
    d = cast<TemplateTypeParmType>(tp)->getDecl();
    break;
  case Type::TemplateSpecialization:
    if (specialization)
      *specialization = true;
    if (const RecordType *record = tp->getAs<RecordType>())
      d = record->getDecl();
    else
      d = cast<TemplateSpecializationType>(tp)
              ->getTemplateName()
              .getAsTemplateDecl();
    break;

  case Type::Auto:
  case Type::DeducedTemplateSpecialization:
    tp = cast<DeducedType>(tp)->getDeducedType().getTypePtrOrNull();
    if (tp)
      goto try_again;
    break;

  case Type::InjectedClassName:
    d = cast<InjectedClassNameType>(tp)->getDecl();
    break;

    // FIXME: Template type parameters!

  case Type::Elaborated:
    tp = cast<ElaboratedType>(tp)->getNamedType().getTypePtrOrNull();
    goto try_again;

  default:
    break;
  }
  return d;
}

const Decl *getAdjustedDecl(const Decl *d) {
  while (d) {
    if (auto *r = dyn_cast<CXXRecordDecl>(d)) {
      if (auto *s = dyn_cast<ClassTemplateSpecializationDecl>(r)) {
        if (!s->getTypeAsWritten()) {
          llvm::PointerUnion<ClassTemplateDecl *,
                             ClassTemplatePartialSpecializationDecl *>
              result = s->getSpecializedTemplateOrPartial();
          if (result.is<ClassTemplateDecl *>())
            d = result.get<ClassTemplateDecl *>();
          else
            d = result.get<ClassTemplatePartialSpecializationDecl *>();
          continue;
        }
      } else if (auto *d1 = r->getInstantiatedFromMemberClass()) {
        d = d1;
        continue;
      }
    } else if (auto *ed = dyn_cast<EnumDecl>(d)) {
      if (auto *d1 = ed->getInstantiatedFromMemberEnum()) {
        d = d1;
        continue;
      }
    }
    break;
  }
  return d;
}

bool validateRecord(const RecordDecl *rd) {
  for (const auto *i : rd->fields()) {
    QualType fqt = i->getType();
    if (fqt->isIncompleteType() || fqt->isDependentType())
      return false;
    if (const RecordType *childType = i->getType()->getAs<RecordType>())
      if (const RecordDecl *child = childType->getDecl())
        if (!validateRecord(child))
          return false;
  }
  return true;
}

class IndexDataConsumer : public index::IndexDataConsumer {
public:
  ASTContext *ctx;
  IndexParam &param;

  std::string getComment(const Decl *d) {
    SourceManager &sm = ctx->getSourceManager();
    const RawComment *rc = ctx->getRawCommentForAnyRedecl(d);
    if (!rc)
      return "";
    StringRef raw = rc->getRawText(ctx->getSourceManager());
    SourceRange sr = rc->getSourceRange();
    std::pair<FileID, unsigned> bInfo = sm.getDecomposedLoc(sr.getBegin());
    unsigned start_column = sm.getLineNumber(bInfo.first, bInfo.second);
    std::string ret;
    int pad = -1;
    for (const char *p = raw.data(), *e = raw.end(); p < e;) {
      // The first line starts with a comment marker, but the rest needs
      // un-indenting.
      unsigned skip = start_column - 1;
      for (; skip > 0 && p < e && (*p == ' ' || *p == '\t'); p++)
        skip--;
      const char *q = p;
      while (q < e && *q != '\n')
        q++;
      if (q < e)
        q++;
      // A minimalist approach to skip Doxygen comment markers.
      // See https://www.stack.nl/~dimitri/doxygen/manual/docblocks.html
      if (pad < 0) {
        // First line, detect the length of comment marker and put into |pad|
        const char *begin = p;
        while (p < e && (*p == '/' || *p == '*' || *p == '-' || *p == '='))
          p++;
        if (p < e && (*p == '<' || *p == '!'))
          p++;
        if (p < e && *p == ' ')
          p++;
        if (p + 1 == q)
          p++;
        else
          pad = int(p - begin);
      } else {
        // Other lines, skip |pad| bytes
        int prefix = pad;
        while (prefix > 0 && p < e &&
               (*p == ' ' || *p == '/' || *p == '*' || *p == '<' || *p == '!'))
          prefix--, p++;
      }
      ret.insert(ret.end(), p, q);
      p = q;
    }
    while (ret.size() && isspace(ret.back()))
      ret.pop_back();
    if (StringRef(ret).endswith("*/") || StringRef(ret).endswith("\n/"))
      ret.resize(ret.size() - 2);
    while (ret.size() && isspace(ret.back()))
      ret.pop_back();
    return ret;
  }

  Usr getUsr(const Decl *d, IndexParam::DeclInfo **info = nullptr) const {
    d = d->getCanonicalDecl();
    auto [it, inserted] = param.decl2Info.try_emplace(d);
    if (inserted) {
      SmallString<256> usr;
      index::generateUSRForDecl(d, usr);
      auto &info = it->second;
      info.usr = hashUsr(usr);
      if (auto *nd = dyn_cast<NamedDecl>(d)) {
        info.short_name = nd->getNameAsString();
        llvm::raw_string_ostream os(info.qualified);
        nd->printQualifiedName(os, getDefaultPolicy());
        simplifyAnonymous(info.qualified);
      }
    }
    if (info)
      *info = &it->second;
    return it->second.usr;
  }

  PrintingPolicy getDefaultPolicy() const {
    PrintingPolicy pp(ctx->getLangOpts());
    pp.AnonymousTagLocations = false;
    pp.TerseOutput = true;
    pp.PolishForDeclaration = true;
    pp.ConstantsAsWritten = true;
    pp.SuppressTagKeyword = true;
    pp.SuppressUnwrittenScope = g_config->index.name.suppressUnwrittenScope;
    pp.SuppressInitializers = true;
    pp.FullyQualifiedName = false;
    return pp;
  }

  static void simplifyAnonymous(std::string &name) {
    for (std::string::size_type i = 0;;) {
      if ((i = name.find("(anonymous ", i)) == std::string::npos)
        break;
      i++;
      if (name.size() - i > 19 && name.compare(i + 10, 9, "namespace") == 0)
        name.replace(i, 19, "anon ns");
      else
        name.replace(i, 9, "anon");
    }
  }

  template <typename Def>
  void setName(const Decl *d, std::string_view short_name,
               std::string_view qualified, Def &def) {
    SmallString<256> str;
    llvm::raw_svector_ostream os(str);
    d->print(os, getDefaultPolicy());

    std::string name(str.data(), str.size());
    simplifyAnonymous(name);
    // Remove \n in DeclPrinter.cpp "{\n" + if(!TerseOutput)something + "}"
    for (std::string::size_type i = 0;;) {
      if ((i = name.find("{\n}", i)) == std::string::npos)
        break;
      name.replace(i, 3, "{}");
    }
    auto i = name.find(short_name);
    if (short_name.size())
      while (i != std::string::npos &&
             ((i && isIdentifierBody(name[i - 1])) ||
              isIdentifierBody(name[i + short_name.size()])))
        i = name.find(short_name, i + short_name.size());
    if (i == std::string::npos) {
      // e.g. operator type-parameter-1
      i = 0;
      def.short_name_offset = 0;
    } else if (short_name.empty() || (i >= 2 && name[i - 2] == ':')) {
      // Don't replace name with qualified name in ns::name Cls::*name
      def.short_name_offset = i;
    } else {
      name.replace(i, short_name.size(), qualified);
      def.short_name_offset = i + qualified.size() - short_name.size();
    }
    def.short_name_size = short_name.size();
    for (int paren = 0; i; i--) {
      // Skip parentheses in "(anon struct)::name"
      if (name[i - 1] == ')')
        paren++;
      else if (name[i - 1] == '(')
        paren--;
      else if (!(paren > 0 || isIdentifierBody(name[i - 1]) ||
                 name[i - 1] == ':'))
        break;
    }
    def.qual_name_offset = i;
    def.detailed_name = intern(name);
  }

  void setVarName(const Decl *d, std::string_view short_name,
                  std::string_view qualified, IndexVar::Def &def) {
    QualType t;
    const Expr *init = nullptr;
    bool deduced = false;
    if (auto *vd = dyn_cast<VarDecl>(d)) {
      t = vd->getType();
      init = vd->getAnyInitializer();
      def.storage = vd->getStorageClass();
    } else if (auto *fd = dyn_cast<FieldDecl>(d)) {
      t = fd->getType();
      init = fd->getInClassInitializer();
    } else if (auto *bd = dyn_cast<BindingDecl>(d)) {
      t = bd->getType();
      deduced = true;
    }
    if (!t.isNull()) {
      if (t->getContainedDeducedType()) {
        deduced = true;
      } else if (auto *dt = dyn_cast<DecltypeType>(t)) {
        // decltype(y) x;
        while (dt && !dt->getUnderlyingType().isNull()) {
          t = dt->getUnderlyingType();
          dt = dyn_cast<DecltypeType>(t);
        }
        deduced = true;
      }
    }
    if (!t.isNull() && deduced) {
      SmallString<256> str;
      llvm::raw_svector_ostream os(str);
      PrintingPolicy pp = getDefaultPolicy();
      t.print(os, pp);
      if (str.size() &&
          (str.back() != ' ' && str.back() != '*' && str.back() != '&'))
        str += ' ';
      def.qual_name_offset = str.size();
      def.short_name_offset = str.size() + qualified.size() - short_name.size();
      def.short_name_size = short_name.size();
      str += StringRef(qualified.data(), qualified.size());
      def.detailed_name = intern(str);
    } else {
      setName(d, short_name, qualified, def);
    }
    if (init) {
      SourceManager &sm = ctx->getSourceManager();
      const LangOptions &lang = ctx->getLangOpts();
      SourceRange sr =
          sm.getExpansionRange(init->getSourceRange()).getAsRange();
      SourceLocation l = d->getLocation();
      if (l.isMacroID() || !sm.isBeforeInTranslationUnit(l, sr.getBegin()))
        return;
      StringRef buf = getSourceInRange(sm, lang, sr);
      Twine init = buf.count('\n') <= g_config->index.maxInitializerLines - 1
                       ? buf.size() && buf[0] == ':' ? Twine(" ", buf)
                                                     : Twine(" = ", buf)
                       : Twine();
      Twine t = def.detailed_name + init;
      def.hover =
          def.storage == SC_Static && strncmp(def.detailed_name, "static ", 7)
              ? intern(("static " + t).str())
              : intern(t.str());
    }
  }

  static int getFileLID(IndexFile *db, SourceManager &sm, const FileEntry &fe) {
    auto [it, inserted] = db->uid2lid_and_path.try_emplace(fe.getUniqueID());
    if (inserted) {
      it->second.first = db->uid2lid_and_path.size() - 1;
      SmallString<256> path = fe.tryGetRealPathName();
      if (path.empty())
        path = fe.getName();
      if (!llvm::sys::path::is_absolute(path) &&
          !sm.getFileManager().makeAbsolutePath(path))
        return -1;
      it->second.second = llvm::sys::path::convert_to_slash(path.str());
    }
    return it->second.first;
  }

  void addMacroUse(IndexFile *db, SourceManager &sm, Usr usr, Kind kind,
                   SourceLocation sl) const {
    const FileEntry *FE = sm.getFileEntryForID(sm.getFileID(sl));
    if (!FE)
      return;
    int lid = getFileLID(db, sm, *FE);
    if (lid < 0)
      return;
    Range spell = fromTokenRange(sm, ctx->getLangOpts(), SourceRange(sl, sl));
    Use use{{spell, Role::Dynamic}, lid};
    switch (kind) {
    case Kind::Func:
      db->toFunc(usr).uses.push_back(use);
      break;
    case Kind::Type:
      db->toType(usr).uses.push_back(use);
      break;
    case Kind::Var:
      db->toVar(usr).uses.push_back(use);
      break;
    default:
      llvm_unreachable("");
    }
  }

  void collectRecordMembers(IndexType &type, const RecordDecl *rd) {
    SmallVector<std::pair<const RecordDecl *, int>, 2> stack{{rd, 0}};
    llvm::DenseSet<const RecordDecl *> seen;
    seen.insert(rd);
    while (stack.size()) {
      int offset;
      std::tie(rd, offset) = stack.back();
      stack.pop_back();
      if (!rd->isCompleteDefinition() || rd->isDependentType() ||
          rd->isInvalidDecl() || !validateRecord(rd))
        offset = -1;
      for (FieldDecl *fd : rd->fields()) {
        int offset1 = offset < 0 ? -1 : offset + ctx->getFieldOffset(fd);
        if (fd->getIdentifier())
          type.def.vars.emplace_back(getUsr(fd), offset1);
        else if (const auto *rt1 = fd->getType()->getAs<RecordType>()) {
          if (const RecordDecl *rd1 = rt1->getDecl())
            if (seen.insert(rd1).second)
              stack.push_back({rd1, offset1});
        }
      }
    }
  }

public:
  IndexDataConsumer(IndexParam &param) : param(param) {}
  void initialize(ASTContext &ctx) override { this->ctx = param.ctx = &ctx; }
#if LLVM_VERSION_MAJOR < 10 // llvmorg-10-init-12036-g3b9715cb219
# define handleDeclOccurrence handleDeclOccurence
#endif
  bool handleDeclOccurrence(const Decl *d, index::SymbolRoleSet roles,
                            ArrayRef<index::SymbolRelation> relations,
                            SourceLocation src_loc,
                            ASTNodeInfo ast_node) override {
    if (!param.no_linkage) {
      if (auto *nd = dyn_cast<NamedDecl>(d); nd && nd->hasLinkage())
        ;
      else
        return true;
    }
    SourceManager &sm = ctx->getSourceManager();
    const LangOptions &lang = ctx->getLangOpts();
    FileID fid;
    SourceLocation spell = sm.getSpellingLoc(src_loc);
    const FileEntry *fe;
    Range loc;
    auto r = sm.isMacroArgExpansion(src_loc)
                 ? CharSourceRange::getTokenRange(spell)
                 : sm.getExpansionRange(src_loc);
    loc = fromCharSourceRange(sm, lang, r);
    fid = sm.getFileID(r.getBegin());
    fe = sm.getFileEntryForID(fid);
    if (!fe)
      return true;
    int lid = -1;
    IndexFile *db;
    if (g_config->index.multiVersion && param.useMultiVersion(*fe)) {
      db = param.consumeFile(*sm.getFileEntryForID(sm.getMainFileID()));
      if (!db)
        return true;
      param.seenFile(*fe);
      if (!sm.isWrittenInMainFile(r.getBegin()))
        lid = getFileLID(db, sm, *fe);
    } else {
      db = param.consumeFile(*fe);
      if (!db)
        return true;
    }

    // spell, extent, comments use OrigD while most others use adjusted |D|.
    const Decl *origD = ast_node.OrigD;
    const DeclContext *sem_dc = origD->getDeclContext()->getRedeclContext();
    const DeclContext *lex_dc = ast_node.ContainerDC->getRedeclContext();
    {
      const NamespaceDecl *nd;
      while ((nd = dyn_cast<NamespaceDecl>(cast<Decl>(sem_dc))) &&
             nd->isAnonymousNamespace())
        sem_dc = nd->getDeclContext()->getRedeclContext();
      while ((nd = dyn_cast<NamespaceDecl>(cast<Decl>(lex_dc))) &&
             nd->isAnonymousNamespace())
        lex_dc = nd->getDeclContext()->getRedeclContext();
    }
    Role role = static_cast<Role>(roles);
    db->language = LanguageId((int)db->language | (int)getDeclLanguage(d));

    bool is_decl = roles & uint32_t(index::SymbolRole::Declaration);
    bool is_def = roles & uint32_t(index::SymbolRole::Definition);
    if (is_decl && d->getKind() == Decl::Binding)
      is_def = true;
    IndexFunc *func = nullptr;
    IndexType *type = nullptr;
    IndexVar *var = nullptr;
    SymbolKind ls_kind = SymbolKind::Unknown;
    Kind kind = getKind(d, ls_kind);

    if (is_def)
      switch (d->getKind()) {
      case Decl::CXXConversion: // *operator* int => *operator int*
      case Decl::CXXDestructor: // *~*A => *~A*
      case Decl::CXXMethod:     // *operator*= => *operator=*
      case Decl::Function:      // operator delete
        if (src_loc.isFileID()) {
          SourceRange sr =
              cast<FunctionDecl>(origD)->getNameInfo().getSourceRange();
          if (sr.getEnd().isFileID())
            loc = fromTokenRange(sm, lang, sr);
        }
        break;
      default:
        break;
      }
    else {
      // e.g. typedef Foo<int> gg; => Foo has an unadjusted `D`
      const Decl *d1 = getAdjustedDecl(d);
      if (d1 && d1 != d)
        d = d1;
    }

    IndexParam::DeclInfo *info;
    Usr usr = getUsr(d, &info);

    auto do_def_decl = [&](auto *entity) {
      Use use{{loc, role}, lid};
      if (is_def) {
        SourceRange sr = origD->getSourceRange();
        entity->def.spell = {use,
                             fromTokenRangeDefaulted(sm, lang, sr, fe, loc)};
        entity->def.parent_kind = SymbolKind::File;
        getKind(cast<Decl>(sem_dc), entity->def.parent_kind);
      } else if (is_decl) {
        SourceRange sr = origD->getSourceRange();
        entity->declarations.push_back(
            {use, fromTokenRangeDefaulted(sm, lang, sr, fe, loc)});
      } else {
        entity->uses.push_back(use);
        return;
      }
      if (entity->def.comments[0] == '\0' && g_config->index.comments)
        entity->def.comments = intern(getComment(origD));
    };
    switch (kind) {
    case Kind::Invalid:
      if (ls_kind == SymbolKind::Unknown)
        LOG_S(INFO) << "Unhandled " << int(d->getKind()) << " "
                    << info->qualified << " in " << db->path << ":"
                    << (loc.start.line + 1) << ":" << (loc.start.column + 1);
      return true;
    case Kind::File:
      return true;
    case Kind::Func:
      func = &db->toFunc(usr);
      func->def.kind = ls_kind;
      // Mark as Role::Implicit to span one more column to the left/right.
      if (!is_def && !is_decl &&
          (d->getKind() == Decl::CXXConstructor ||
           d->getKind() == Decl::CXXConversion))
        role = Role(role | Role::Implicit);
      do_def_decl(func);
      if (spell != src_loc)
        addMacroUse(db, sm, usr, Kind::Func, spell);
      if (func->def.detailed_name[0] == '\0')
        setName(d, info->short_name, info->qualified, func->def);
      if (is_def || is_decl) {
        const Decl *dc = cast<Decl>(sem_dc);
        if (getKind(dc, ls_kind) == Kind::Type)
          db->toType(getUsr(dc)).def.funcs.push_back(usr);
      } else {
        const Decl *dc = cast<Decl>(lex_dc);
        if (getKind(dc, ls_kind) == Kind::Func)
          db->toFunc(getUsr(dc))
              .def.callees.push_back({loc, usr, Kind::Func, role});
      }
      break;
    case Kind::Type:
      type = &db->toType(usr);
      type->def.kind = ls_kind;
      do_def_decl(type);
      if (spell != src_loc)
        addMacroUse(db, sm, usr, Kind::Type, spell);
      if ((is_def || type->def.detailed_name[0] == '\0') &&
          info->short_name.size()) {
        if (d->getKind() == Decl::TemplateTypeParm)
          type->def.detailed_name = intern(info->short_name);
        else
          // OrigD may be detailed, e.g. "struct D : B {}"
          setName(origD, info->short_name, info->qualified, type->def);
      }
      if (is_def || is_decl) {
        const Decl *dc = cast<Decl>(sem_dc);
        if (getKind(dc, ls_kind) == Kind::Type)
          db->toType(getUsr(dc)).def.types.push_back(usr);
      }
      break;
    case Kind::Var:
      var = &db->toVar(usr);
      var->def.kind = ls_kind;
      do_def_decl(var);
      if (spell != src_loc)
        addMacroUse(db, sm, usr, Kind::Var, spell);
      if (var->def.detailed_name[0] == '\0')
        setVarName(d, info->short_name, info->qualified, var->def);
      QualType t;
      if (auto *vd = dyn_cast<ValueDecl>(d))
        t = vd->getType();
      if (is_def || is_decl) {
        const Decl *dc = cast<Decl>(sem_dc);
        Kind kind = getKind(dc, var->def.parent_kind);
        if (kind == Kind::Func)
          db->toFunc(getUsr(dc)).def.vars.push_back(usr);
        else if (kind == Kind::Type && !isa<RecordDecl>(sem_dc))
          db->toType(getUsr(dc)).def.vars.emplace_back(usr, -1);
        if (!t.isNull()) {
          if (auto *bt = t->getAs<BuiltinType>()) {
            Usr usr1 = static_cast<Usr>(bt->getKind());
            var->def.type = usr1;
            if (!isa<EnumConstantDecl>(d))
              db->toType(usr1).instances.push_back(usr);
          } else if (const Decl *d1 = getAdjustedDecl(getTypeDecl(t))) {
#if LLVM_VERSION_MAJOR < 9
            if (isa<TemplateTypeParmDecl>(d1)) {
              // e.g. TemplateTypeParmDecl is not handled by
              // handleDeclOccurence.
              SourceRange sr1 = d1->getSourceRange();
              if (sm.getFileID(sr1.getBegin()) == fid) {
                IndexParam::DeclInfo *info1;
                Usr usr1 = getUsr(d1, &info1);
                IndexType &type1 = db->toType(usr1);
                SourceLocation sl1 = d1->getLocation();
                type1.def.spell = {
                    Use{{fromTokenRange(sm, lang, {sl1, sl1}), Role::Definition},
                        lid},
                    fromTokenRange(sm, lang, sr1)};
                type1.def.detailed_name = intern(info1->short_name);
                type1.def.short_name_size = int16_t(info1->short_name.size());
                type1.def.kind = SymbolKind::TypeParameter;
                type1.def.parent_kind = SymbolKind::Class;
                var->def.type = usr1;
                type1.instances.push_back(usr);
                break;
              }
            }
#endif

            IndexParam::DeclInfo *info1;
            Usr usr1 = getUsr(d1, &info1);
            var->def.type = usr1;
            if (!isa<EnumConstantDecl>(d))
              db->toType(usr1).instances.push_back(usr);
          }
        }
      } else if (!var->def.spell && var->declarations.empty()) {
        // e.g. lambda parameter
        SourceLocation l = d->getLocation();
        if (sm.getFileID(l) == fid) {
          var->def.spell = {
              Use{{fromTokenRange(sm, lang, {l, l}), Role::Definition}, lid},
              fromTokenRange(sm, lang, d->getSourceRange())};
          var->def.parent_kind = SymbolKind::Method;
        }
      }
      break;
    }

    switch (d->getKind()) {
    case Decl::Namespace:
      if (d->isFirstDecl()) {
        auto *nd = cast<NamespaceDecl>(d);
        auto *nd1 = cast<Decl>(nd->getParent());
        if (isa<NamespaceDecl>(nd1)) {
          Usr usr1 = getUsr(nd1);
          type->def.bases.push_back(usr1);
          db->toType(usr1).derived.push_back(usr);
        }
      }
      break;
    case Decl::NamespaceAlias: {
      auto *nad = cast<NamespaceAliasDecl>(d);
      if (const NamespaceDecl *nd = nad->getNamespace()) {
        Usr usr1 = getUsr(nd);
        type->def.alias_of = usr1;
        (void)db->toType(usr1);
      }
      break;
    }
    case Decl::CXXRecord:
      if (is_def) {
        auto *rd = dyn_cast<CXXRecordDecl>(d);
        if (rd && rd->hasDefinition())
          for (const CXXBaseSpecifier &base : rd->bases())
            if (const Decl *baseD =
                    getAdjustedDecl(getTypeDecl(base.getType()))) {
              Usr usr1 = getUsr(baseD);
              type->def.bases.push_back(usr1);
              db->toType(usr1).derived.push_back(usr);
            }
      }
      [[fallthrough]];
    case Decl::Record:
      if (auto *rd = dyn_cast<RecordDecl>(d)) {
        if (type->def.detailed_name[0] == '\0' && info->short_name.empty()) {
          StringRef tag;
          switch (rd->getTagKind()) {
          case TTK_Struct:
            tag = "struct";
            break;
          case TTK_Interface:
            tag = "__interface";
            break;
          case TTK_Union:
            tag = "union";
            break;
          case TTK_Class:
            tag = "class";
            break;
          case TTK_Enum:
            tag = "enum";
            break;
          }
          if (TypedefNameDecl *td = rd->getTypedefNameForAnonDecl()) {
            StringRef name = td->getName();
            std::string detailed = ("anon " + tag + " " + name).str();
            type->def.detailed_name = intern(detailed);
            type->def.short_name_size = detailed.size();
          } else {
            std::string name = ("anon " + tag).str();
            type->def.detailed_name = intern(name);
            type->def.short_name_size = name.size();
          }
        }
        if (is_def)
          if (auto *ord = dyn_cast<RecordDecl>(origD))
            collectRecordMembers(*type, ord);
      }
      break;
    case Decl::ClassTemplateSpecialization:
    case Decl::ClassTemplatePartialSpecialization:
      type->def.kind = SymbolKind::Class;
      if (is_def) {
        if (auto *ord = dyn_cast<RecordDecl>(origD))
          collectRecordMembers(*type, ord);
        if (auto *rd = dyn_cast<CXXRecordDecl>(d)) {
          Decl *d1 = nullptr;
          if (auto *sd = dyn_cast<ClassTemplatePartialSpecializationDecl>(rd))
            d1 = sd->getSpecializedTemplate();
          else if (auto *sd = dyn_cast<ClassTemplateSpecializationDecl>(rd)) {
            llvm::PointerUnion<ClassTemplateDecl *,
                               ClassTemplatePartialSpecializationDecl *>
                result = sd->getSpecializedTemplateOrPartial();
            if (result.is<ClassTemplateDecl *>())
              d1 = result.get<ClassTemplateDecl *>();
            else
              d1 = result.get<ClassTemplatePartialSpecializationDecl *>();

          } else
            d1 = rd->getInstantiatedFromMemberClass();
          if (d1) {
            Usr usr1 = getUsr(d1);
            type->def.bases.push_back(usr1);
            db->toType(usr1).derived.push_back(usr);
          }
        }
      }
      break;
    case Decl::TypeAlias:
    case Decl::Typedef:
    case Decl::UnresolvedUsingTypename:
      if (auto *td = dyn_cast<TypedefNameDecl>(d)) {
        bool specialization = false;
        QualType t = td->getUnderlyingType();
        if (const Decl *d1 = getAdjustedDecl(getTypeDecl(t, &specialization))) {
          Usr usr1 = getUsr(d1);
          IndexType &type1 = db->toType(usr1);
          type->def.alias_of = usr1;
          // Not visited template<class T> struct B {typedef A<T> t;};
          if (specialization) {
            const TypeSourceInfo *tsi = td->getTypeSourceInfo();
            SourceLocation l1 = tsi->getTypeLoc().getBeginLoc();
            if (sm.getFileID(l1) == fid)
              type1.uses.push_back(
                  {{fromTokenRange(sm, lang, {l1, l1}), Role::Reference}, lid});
          }
        }
      }
      break;
    case Decl::CXXMethod:
      if (is_def || is_decl) {
        if (auto *nd = dyn_cast<NamedDecl>(d)) {
          SmallVector<const NamedDecl *, 8> overDecls;
          ctx->getOverriddenMethods(nd, overDecls);
          for (const auto *nd1 : overDecls) {
            Usr usr1 = getUsr(nd1);
            func->def.bases.push_back(usr1);
            db->toFunc(usr1).derived.push_back(usr);
          }
        }
      }
      break;
    case Decl::EnumConstant:
      if (is_def && strchr(var->def.detailed_name, '=') == nullptr) {
        auto *ecd = cast<EnumConstantDecl>(d);
        const auto &val = ecd->getInitVal();
        std::string init =
            " = " + (val.isSigned() ? std::to_string(val.getSExtValue())
                                    : std::to_string(val.getZExtValue()));
        var->def.hover = intern(var->def.detailed_name + init);
      }
      break;
    default:
      break;
    }
    return true;
  }
};

class IndexPPCallbacks : public PPCallbacks {
  SourceManager &sm;
  IndexParam &param;

  std::pair<StringRef, Usr> getMacro(const Token &tok) const {
    StringRef name = tok.getIdentifierInfo()->getName();
    SmallString<256> usr("@macro@");
    usr += name;
    return {name, hashUsr(usr)};
  }

public:
  IndexPPCallbacks(SourceManager &sm, IndexParam &param)
      : sm(sm), param(param) {}
  void InclusionDirective(SourceLocation hashLoc, const Token &tok,
                          StringRef included, bool isAngled,
                          CharSourceRange filenameRange, const FileEntry *file,
                          StringRef searchPath, StringRef relativePath,
                          const Module *imported,
                          SrcMgr::CharacteristicKind fileType) override {
    if (!file)
      return;
    auto spell = fromCharSourceRange(sm, param.ctx->getLangOpts(),
                                     filenameRange, nullptr);
    const FileEntry *fe =
        sm.getFileEntryForID(sm.getFileID(filenameRange.getBegin()));
    if (!fe)
      return;
    if (IndexFile *db = param.consumeFile(*fe)) {
      std::string path = pathFromFileEntry(*file);
      if (path.size())
        db->includes.push_back({spell.start.line, intern(path)});
    }
  }
  void MacroDefined(const Token &tok, const MacroDirective *md) override {
    const LangOptions &lang = param.ctx->getLangOpts();
    SourceLocation sl = md->getLocation();
    const FileEntry *fe = sm.getFileEntryForID(sm.getFileID(sl));
    if (!fe)
      return;
    if (IndexFile *db = param.consumeFile(*fe)) {
      auto [name, usr] = getMacro(tok);
      IndexVar &var = db->toVar(usr);
      Range range = fromTokenRange(sm, lang, {sl, sl}, nullptr);
      var.def.kind = SymbolKind::Macro;
      var.def.parent_kind = SymbolKind::File;
      if (var.def.spell)
        var.declarations.push_back(*var.def.spell);
      const MacroInfo *mi = md->getMacroInfo();
      SourceRange sr(mi->getDefinitionLoc(), mi->getDefinitionEndLoc());
      Range extent = fromTokenRange(sm, param.ctx->getLangOpts(), sr);
      var.def.spell = {Use{{range, Role::Definition}}, extent};
      if (var.def.detailed_name[0] == '\0') {
        var.def.detailed_name = intern(name);
        var.def.short_name_size = name.size();
        StringRef buf = getSourceInRange(sm, lang, sr);
        var.def.hover =
            intern(buf.count('\n') <= g_config->index.maxInitializerLines - 1
                       ? Twine("#define ", getSourceInRange(sm, lang, sr)).str()
                       : Twine("#define ", name).str());
      }
    }
  }
  void MacroExpands(const Token &tok, const MacroDefinition &, SourceRange sr,
                    const MacroArgs *) override {
    llvm::sys::fs::UniqueID uniqueID;
    SourceLocation sl = sm.getSpellingLoc(sr.getBegin());
    const FileEntry *fe = sm.getFileEntryForID(sm.getFileID(sl));
    if (!fe)
      return;
    if (IndexFile *db = param.consumeFile(*fe)) {
      IndexVar &var = db->toVar(getMacro(tok).second);
      var.uses.push_back(
          {{fromTokenRange(sm, param.ctx->getLangOpts(), {sl, sl}, &uniqueID),
            Role::Dynamic}});
    }
  }
  void MacroUndefined(const Token &tok, const MacroDefinition &md,
                      const MacroDirective *ud) override {
    if (ud) {
      SourceLocation sl = ud->getLocation();
      MacroExpands(tok, md, {sl, sl}, nullptr);
    }
  }
  void SourceRangeSkipped(SourceRange sr, SourceLocation) override {
    Range range = fromCharSourceRange(sm, param.ctx->getLangOpts(),
                                      CharSourceRange::getCharRange(sr));
    if (const FileEntry *fe = sm.getFileEntryForID(sm.getFileID(sr.getBegin())))
      if (IndexFile *db = param.consumeFile(*fe))
        db->skipped_ranges.push_back(range);
  }
};

class IndexFrontendAction : public ASTFrontendAction {
  IndexParam &param;

public:
  IndexFrontendAction(IndexParam &param) : param(param) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override {
    Preprocessor &PP = CI.getPreprocessor();
    PP.addPPCallbacks(
        std::make_unique<IndexPPCallbacks>(PP.getSourceManager(), param));
    return std::make_unique<ASTConsumer>();
  }
};
} // namespace

const int IndexFile::kMajorVersion = 21;
const int IndexFile::kMinorVersion = 0;

IndexFile::IndexFile(const std::string &path, const std::string &contents,
                     bool no_linkage)
    : path(path), no_linkage(no_linkage), file_contents(contents) {}

IndexFunc &IndexFile::toFunc(Usr usr) {
  auto [it, inserted] = usr2func.try_emplace(usr);
  if (inserted)
    it->second.usr = usr;
  return it->second;
}

IndexType &IndexFile::toType(Usr usr) {
  auto [it, inserted] = usr2type.try_emplace(usr);
  if (inserted)
    it->second.usr = usr;
  return it->second;
}

IndexVar &IndexFile::toVar(Usr usr) {
  auto [it, inserted] = usr2var.try_emplace(usr);
  if (inserted)
    it->second.usr = usr;
  return it->second;
}

std::string IndexFile::toString() {
  return ccls::serialize(SerializeFormat::Json, *this);
}

template <typename T> void uniquify(std::vector<T> &a) {
  std::unordered_set<T> seen;
  size_t n = 0;
  for (size_t i = 0; i < a.size(); i++)
    if (seen.insert(a[i]).second)
      a[n++] = a[i];
  a.resize(n);
}

namespace idx {
void init() {
  multiVersionMatcher = new GroupMatch(g_config->index.multiVersionWhitelist,
                                       g_config->index.multiVersionBlacklist);
}

std::vector<std::unique_ptr<IndexFile>>
index(SemaManager *manager, WorkingFiles *wfiles, VFS *vfs,
      const std::string &opt_wdir, const std::string &main,
      const std::vector<const char *> &args,
      const std::vector<std::pair<std::string, std::string>> &remapped,
      bool no_linkage, bool &ok) {
  ok = true;
  auto pch = std::make_shared<PCHContainerOperations>();
  llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs =
      llvm::vfs::getRealFileSystem();
  std::shared_ptr<CompilerInvocation> ci =
      buildCompilerInvocation(main, args, fs);
  // e.g. .s
  if (!ci)
    return {};
  ok = false;
  // -fparse-all-comments enables documentation in the indexer and in
  // code completion.
  ci->getLangOpts()->CommentOpts.ParseAllComments =
      g_config->index.comments > 1;
  ci->getLangOpts()->RetainCommentsFromSystemHeaders = true;
  std::string buf = wfiles->getContent(main);
  std::vector<std::unique_ptr<llvm::MemoryBuffer>> bufs;
  if (buf.size())
    for (auto &[filename, content] : remapped) {
      bufs.push_back(llvm::MemoryBuffer::getMemBuffer(content));
      ci->getPreprocessorOpts().addRemappedFile(filename, bufs.back().get());
    }

  DiagnosticConsumer dc;
  auto clang = std::make_unique<CompilerInstance>(pch);
  clang->setInvocation(std::move(ci));
  clang->createDiagnostics(&dc, false);
  clang->setTarget(TargetInfo::CreateTargetInfo(
      clang->getDiagnostics(), clang->getInvocation().TargetOpts));
  if (!clang->hasTarget())
    return {};
  clang->getPreprocessorOpts().RetainRemappedFileBuffers = true;
#if LLVM_VERSION_MAJOR >= 9 // rC357037
  clang->createFileManager(fs);
#else
  clang->setVirtualFileSystem(fs);
  clang->createFileManager();
#endif
  clang->setSourceManager(new SourceManager(clang->getDiagnostics(),
                                            clang->getFileManager(), true));

  IndexParam param(*vfs, no_linkage);
  auto dataConsumer = std::make_shared<IndexDataConsumer>(param);

  index::IndexingOptions indexOpts;
  indexOpts.SystemSymbolFilter =
      index::IndexingOptions::SystemSymbolFilterKind::All;
  if (no_linkage) {
    indexOpts.IndexFunctionLocals = true;
    indexOpts.IndexImplicitInstantiation = true;
#if LLVM_VERSION_MAJOR >= 9

    indexOpts.IndexParametersInDeclarations =
        g_config->index.parametersInDeclarations;
    indexOpts.IndexTemplateParameters = true;
#endif
  }

  std::unique_ptr<FrontendAction> action = createIndexingAction(
      dataConsumer, indexOpts, std::make_unique<IndexFrontendAction>(param));
  std::string reason;
  {
    llvm::CrashRecoveryContext crc;
    auto parse = [&]() {
      if (!action->BeginSourceFile(*clang, clang->getFrontendOpts().Inputs[0]))
        return;
#if LLVM_VERSION_MAJOR >= 9 // rL364464
      if (llvm::Error e = action->Execute()) {
        reason = llvm::toString(std::move(e));
        return;
      }
#else
      if (!action->Execute())
        return;
#endif
      action->EndSourceFile();
      ok = true;
    };
    if (!crc.RunSafely(parse)) {
      LOG_S(ERROR) << "clang crashed for " << main;
      return {};
    }
  }
  if (!ok) {
    LOG_S(ERROR) << "failed to index " << main
                 << (reason.empty() ? "" : ": " + reason);
    return {};
  }

  std::vector<std::unique_ptr<IndexFile>> result;
  for (auto &it : param.uid2file) {
    if (!it.second.db)
      continue;
    std::unique_ptr<IndexFile> &entry = it.second.db;
    entry->import_file = main;
    entry->args = args;
    for (auto &[_, it] : entry->uid2lid_and_path)
      entry->lid2path.emplace_back(it.first, std::move(it.second));
    entry->uid2lid_and_path.clear();
    for (auto &it : entry->usr2func) {
      // e.g. declaration + out-of-line definition
      uniquify(it.second.derived);
      uniquify(it.second.uses);
    }
    for (auto &it : entry->usr2type) {
      uniquify(it.second.derived);
      uniquify(it.second.uses);
      // e.g. declaration + out-of-line definition
      uniquify(it.second.def.bases);
      uniquify(it.second.def.funcs);
    }
    for (auto &it : entry->usr2var)
      uniquify(it.second.uses);

    // Update dependencies for the file.
    for (auto &[_, file] : param.uid2file) {
      const std::string &path = file.path;
      if (path == entry->path)
        entry->mtime = file.mtime;
      else if (path != entry->import_file)
        entry->dependencies[llvm::CachedHashStringRef(intern(path))] =
            file.mtime;
    }
    result.push_back(std::move(entry));
  }

  return result;
}
} // namespace idx

void reflect(JsonReader &vis, SymbolRef &v) {
  std::string t = vis.getString();
  char *s = const_cast<char *>(t.c_str());
  v.range = Range::fromString(s);
  s = strchr(s, '|');
  v.usr = strtoull(s + 1, &s, 10);
  v.kind = static_cast<Kind>(strtol(s + 1, &s, 10));
  v.role = static_cast<Role>(strtol(s + 1, &s, 10));
}
void reflect(JsonReader &vis, Use &v) {
  std::string t = vis.getString();
  char *s = const_cast<char *>(t.c_str());
  v.range = Range::fromString(s);
  s = strchr(s, '|');
  v.role = static_cast<Role>(strtol(s + 1, &s, 10));
  v.file_id = static_cast<int>(strtol(s + 1, &s, 10));
}
void reflect(JsonReader &vis, DeclRef &v) {
  std::string t = vis.getString();
  char *s = const_cast<char *>(t.c_str());
  v.range = Range::fromString(s);
  s = strchr(s, '|') + 1;
  v.extent = Range::fromString(s);
  s = strchr(s, '|');
  v.role = static_cast<Role>(strtol(s + 1, &s, 10));
  v.file_id = static_cast<int>(strtol(s + 1, &s, 10));
}

void reflect(JsonWriter &vis, SymbolRef &v) {
  char buf[99];
  snprintf(buf, sizeof buf, "%s|%" PRIu64 "|%d|%d", v.range.toString().c_str(),
           v.usr, int(v.kind), int(v.role));
  std::string s(buf);
  reflect(vis, s);
}
void reflect(JsonWriter &vis, Use &v) {
  char buf[99];
  snprintf(buf, sizeof buf, "%s|%d|%d", v.range.toString().c_str(), int(v.role),
           v.file_id);
  std::string s(buf);
  reflect(vis, s);
}
void reflect(JsonWriter &vis, DeclRef &v) {
  char buf[99];
  snprintf(buf, sizeof buf, "%s|%s|%d|%d", v.range.toString().c_str(),
           v.extent.toString().c_str(), int(v.role), v.file_id);
  std::string s(buf);
  reflect(vis, s);
}

void reflect(BinaryReader &vis, SymbolRef &v) {
  reflect(vis, v.range);
  reflect(vis, v.usr);
  reflect(vis, v.kind);
  reflect(vis, v.role);
}
void reflect(BinaryReader &vis, Use &v) {
  reflect(vis, v.range);
  reflect(vis, v.role);
  reflect(vis, v.file_id);
}
void reflect(BinaryReader &vis, DeclRef &v) {
  reflect(vis, static_cast<Use &>(v));
  reflect(vis, v.extent);
}

void reflect(BinaryWriter &vis, SymbolRef &v) {
  reflect(vis, v.range);
  reflect(vis, v.usr);
  reflect(vis, v.kind);
  reflect(vis, v.role);
}
void reflect(BinaryWriter &vis, Use &v) {
  reflect(vis, v.range);
  reflect(vis, v.role);
  reflect(vis, v.file_id);
}
void reflect(BinaryWriter &vis, DeclRef &v) {
  reflect(vis, static_cast<Use &>(v));
  reflect(vis, v.extent);
}
} // namespace ccls
