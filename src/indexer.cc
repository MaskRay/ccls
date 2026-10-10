// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "indexer.hh"

#include "clang_tu.hh"
#include "log.hh"
#include "pipeline.hh"
#include "platform.hh"
#include "sema_manager.hh"

#include <clang/AST/AST.h>
#include <clang/AST/ASTDiagnostic.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/Basic/CharInfo.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Index/IndexDataConsumer.h>
#include <clang/Index/IndexingAction.h>
#if LLVM_VERSION_MAJOR >= 23 // llvmorg-22-init-27296-g65cb738ff419
#include <clang/UnifiedSymbolResolution/USRGeneration.h>
#else
#include <clang/Index/USRGeneration.h>
#endif
#include <clang/Lex/PreprocessorOptions.h>
#if LLVM_VERSION_MAJOR >= 22
#include <clang/Sema/HeuristicResolver.h>
#endif
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SaveAndRestore.h>

#include <algorithm>
#include <inttypes.h>
#include <map>
#include <unordered_set>

using namespace clang;

#if LLVM_VERSION_MAJOR >= 18 // llvmorg-18-init-10631-gedd690b02e16
#define TTK_Class TagTypeKind::Class
#define TTK_Enum TagTypeKind::Enum
#define TTK_Interface TagTypeKind::Interface
#define TTK_Struct TagTypeKind::Struct
#define TTK_Union TagTypeKind::Union
#endif

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
  std::unordered_map<FileID, File> uid2file;
  std::unordered_map<FileID, bool> uid2multi;
  struct DeclInfo {
    Usr usr;
    std::string short_name;
    std::string qualified;
  };
  std::unordered_map<const Decl *, DeclInfo> decl2Info;

  // A call to a forwarding function (e.g. std::make_unique<A>). The callee's
  // body is instantiated at the end of the translation unit.
  struct DeferredForward {
    IndexFile *db;
    Range loc;
    Role role;
    int lid;
    const FunctionDecl *callee;
    const Decl *caller; // nullptr if not in a function
  };
  std::vector<DeferredForward> deferred_forward;

  // `auto x = dependent;` in a function template.
  struct DeferredVar {
    IndexFile *db;
    const VarDecl *vd;
    Usr usr;
  };
  std::vector<DeferredVar> deferred_var;

  VFS &vfs;
  ASTContext *ctx;
  bool no_linkage;
  IndexParam(VFS &vfs, bool no_linkage) : vfs(vfs), no_linkage(no_linkage) {}

  void seenFile(FileID fid) {
    // If this is the first time we have seen the file (ignoring if we are
    // generating an index for it):
    auto [it, inserted] = uid2file.try_emplace(fid);
    if (inserted) {
#if LLVM_VERSION_MAJOR < 19
      const FileEntry *fe = ctx->getSourceManager().getFileEntryForID(fid);
#else
      OptionalFileEntryRef fe = ctx->getSourceManager().getFileEntryRefForID(fid);
#endif
      if (!fe)
        return;
      std::string path = pathFromFileEntry(*fe);
      it->second.path = path;
      it->second.mtime = fe->getModificationTime();
      if (!it->second.mtime)
        if (auto tim = lastWriteTime(path))
          it->second.mtime = *tim;
      if (std::optional<std::string> content = readContent(path))
        it->second.content = *content;

      if (!vfs.stamp(path, it->second.mtime, no_linkage ? 3 : 1))
        return;
      it->second.db = std::make_unique<IndexFile>(path, it->second.content, no_linkage);
    }
  }

  IndexFile *consumeFile(FileID fid) {
    seenFile(fid);
    return uid2file[fid].db.get();
  }

  bool useMultiVersion(FileID fid) {
    auto it = uid2multi.try_emplace(fid);
    if (it.second) {
#if LLVM_VERSION_MAJOR < 19
      if (const FileEntry *fe = ctx->getSourceManager().getFileEntryForID(fid))
#else
      if (OptionalFileEntryRef fe = ctx->getSourceManager().getFileEntryRefForID(fid))
#endif
        it.first->second = multiVersionMatcher->matches(pathFromFileEntry(*fe));
    }
    return it.first->second;
  }
};

StringRef getSourceInRange(const SourceManager &sm, const LangOptions &langOpts, SourceRange sr) {
  SourceLocation bloc = sr.getBegin(), eLoc = sr.getEnd();
  std::pair<FileID, unsigned> bInfo = sm.getDecomposedLoc(bloc), eInfo = sm.getDecomposedLoc(eLoc);
  bool invalid = false;
  StringRef buf = sm.getBufferData(bInfo.first, &invalid);
  if (invalid)
    return "";
  return buf.substr(bInfo.second, eInfo.second + Lexer::MeasureTokenLength(eLoc, sm, langOpts) - bInfo.second);
}

Kind getKind(const Decl *d, SymbolKind &kind) {
  switch (d->getKind()) {
  case Decl::LinkageSpec:
    return Kind::Invalid;
  case Decl::Namespace:
  case Decl::NamespaceAlias:
    kind = SymbolKind::Namespace;
    return Kind::Type;
  case Decl::Concept:
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
  case Decl::Var: {
    auto vd = cast<VarDecl>(d);
    if (vd->isStaticDataMember()) {
      kind = SymbolKind::Field;
      return Kind::Var;
    }
    [[fallthrough]];
  }
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
    tp = cast<TypedefType>(tp)->getDecl()->getUnderlyingType().getTypePtrOrNull();
    if (tp)
      goto try_again;
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
      d = cast<TemplateSpecializationType>(tp)->getTemplateName().getAsTemplateDecl();
    break;

  case Type::Auto:
  case Type::DeducedTemplateSpecialization:
    tp = cast<DeducedType>(tp)->getDeducedType().getTypePtrOrNull();
    if (tp)
      goto try_again;
    break;

  case Type::SubstTemplateTypeParm:
    tp = cast<SubstTemplateTypeParmType>(tp)->getReplacementType().getTypePtr();
    goto try_again;

  case Type::InjectedClassName:
    d = cast<InjectedClassNameType>(tp)->getDecl();
    break;

    // FIXME: Template type parameters!

#if LLVM_VERSION_MAJOR < 22 // llvmorg-22-init-3166-g91cdd35008e9
  case Type::Elaborated:
    tp = cast<ElaboratedType>(tp)->getNamedType().getTypePtrOrNull();
    goto try_again;
#endif

  default:
    break;
  }
  return d;
}

// Whether d is a local variable or parameter of a template instantiation. Its
// occurrences duplicate those of the variable in the template pattern, which
// is indexed instead.
bool isInstantiatedLocal(const Decl *d) {
  if (!isa<VarDecl>(d) && !isa<BindingDecl>(d))
    return false;
  // Skip blocks and captured statements (e.g. OpenMP regions), but not lambdas.
  for (const DeclContext *dc = d->getParentFunctionOrMethod(); dc; dc = dc->getParent())
    if (auto *fd = dyn_cast<FunctionDecl>(dc))
      return fd->isTemplateInstantiation();
  return false;
}

// If fd is the pattern of a function template with exactly one instantiation,
// return the instantiation, similar to clangd's getOnlyInstantiation.
const FunctionDecl *getOnlyInstantiation(const FunctionDecl *fd) {
  const FunctionTemplateDecl *ftd = fd ? fd->getDescribedFunctionTemplate() : nullptr;
  if (!ftd)
    return nullptr;
  const FunctionDecl *only = nullptr;
  for (const FunctionDecl *spec : ftd->specializations()) {
    if (spec->getTemplateSpecializationKind() == TSK_ExplicitSpecialization)
      continue;
    if (only)
      return nullptr;
    only = spec;
  }
  return only;
}

// If vd is a local of a function template with exactly one instantiation,
// return the corresponding variable in that instantiation.
const VarDecl *getOnlyInstantiatedVar(const VarDecl *vd) {
  const FunctionDecl *only = getOnlyInstantiation(dyn_cast_or_null<FunctionDecl>(vd->getParentFunctionOrMethod()));
  if (!only)
    return nullptr;
  for (const Decl *d : only->decls())
    if (auto *vd1 = dyn_cast<VarDecl>(d); vd1 && vd1->getLocation() == vd->getLocation())
      return vd1;
  return nullptr;
}

const Decl *getAdjustedDecl(const Decl *d) {
  while (d) {
    if (auto *r = dyn_cast<CXXRecordDecl>(d)) {
      if (auto *s = dyn_cast<ClassTemplateSpecializationDecl>(r)) {
        if (!s->isExplicitSpecialization()) {
          llvm::PointerUnion<ClassTemplateDecl *, ClassTemplatePartialSpecializationDecl *> result =
              s->getSpecializedTemplateOrPartial();
#if LLVM_VERSION_MAJOR >= 15 // llvmorg-15-init-10510-gaab5bd180a42
          if (auto *ctd = dyn_cast<ClassTemplateDecl *>(result))
            d = ctd;
          else
            d = cast<ClassTemplatePartialSpecializationDecl *>(result);
#else
          if (result.is<ClassTemplateDecl *>())
            d = result.get<ClassTemplateDecl *>();
          else
            d = result.get<ClassTemplatePartialSpecializationDecl *>();
#endif
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

// Whether fd is a specialization of a function template whose last parameter is
// `Args &&...` of its own template parameter, e.g. std::make_unique.
bool isLikelyForwardingFunction(const FunctionDecl *fd) {
  const FunctionTemplateDecl *ft = fd->getPrimaryTemplate();
  if (!ft || !ft->getTemplatedDecl()->getNumParams())
    return false;
  const FunctionDecl *pattern = ft->getTemplatedDecl();
  const auto *pet = dyn_cast<PackExpansionType>(pattern->getParamDecl(pattern->getNumParams() - 1)->getType());
  if (!pet)
    return false;
  const auto *ttpt = dyn_cast<TemplateTypeParmType>(pet->getPattern().getNonReferenceType().getTypePtr());
  return ttpt && ft->getTemplateParameters()->getDepth() == ttpt->getDepth();
}

struct ForwardingToConstructorVisitor : RecursiveASTVisitor<ForwardingToConstructorVisitor> {
  llvm::DenseSet<const FunctionDecl *> &seen;
  llvm::SmallVectorImpl<const CXXConstructorDecl *> &out;

  ForwardingToConstructorVisitor(llvm::DenseSet<const FunctionDecl *> &seen,
                                 llvm::SmallVectorImpl<const CXXConstructorDecl *> &out)
      : seen(seen), out(out) {}

  static constexpr unsigned kMaxDepth = 10;

  bool VisitCallExpr(CallExpr *e) {
    if (seen.size() >= kMaxDepth)
      return true;
    FunctionDecl *fd = e->getDirectCallee();
    if (!fd || seen.contains(fd) || !isLikelyForwardingFunction(fd))
      return true;
    seen.insert(fd);
    if (Stmt *body = fd->getBody())
      TraverseStmt(body);
    seen.erase(fd);
    return true;
  }

  bool VisitCXXNewExpr(CXXNewExpr *e) {
    if (const auto *ce = e->getConstructExpr())
      if (auto *cd = ce->getConstructor())
        out.push_back(cd);
    return true;
  }
};

llvm::SmallVector<const CXXConstructorDecl *, 1> searchConstructorsInForwardingFunction(const FunctionDecl *fd) {
  llvm::SmallVector<const CXXConstructorDecl *, 1> ret;
  llvm::DenseSet<const FunctionDecl *> seen{fd};
  if (Stmt *body = fd->getBody())
    ForwardingToConstructorVisitor(seen, ret).TraverseStmt(body);
  return ret;
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

void setPlainAnonymousTag(PrintingPolicy &pp) {
#if LLVM_VERSION_MAJOR >= 23 // llvmorg-23-init-4710-gf5f8435605ba
  pp.AnonymousTagNameStyle = static_cast<unsigned>(PrintingPolicy::AnonymousTagMode::Plain);
#else
  pp.AnonymousTagLocations = false;
#endif
}

// The following inlay hint code is ported from clangd's InlayHints.cpp.

#if LLVM_VERSION_MAJOR >= 16 // llvmorg-16-init-7826-gbcd9ba2b7e64
using PackId = const TemplateTypeParmDecl *;
#else
using PackId = const TemplateTypeParmType *;
#endif

// If pvd is declared as `Args`, `Args &` or `Args &&` of an expanded template
// parameter pack, return the pack.
PackId getUnderlyingPack(const ParmVarDecl *pvd) {
  const Type *t = pvd->getType().getTypePtr();
  if (auto *rt = dyn_cast<ReferenceType>(t))
    t = rt->getPointeeTypeAsWritten().getTypePtr();
  if (auto *st = dyn_cast<SubstTemplateTypeParmType>(t))
    if (PackId replaced = st->getReplacedParameter(); replaced->isParameterPack())
      return replaced;
  return nullptr;
}

bool isExpandedFromParameterPack(const ParmVarDecl *pvd) { return getUnderlyingPack(pvd); }

// Return the last template parameter pack of the function template fd is
// specialized from.
PackId getFunctionPack(const FunctionDecl *fd) {
  if (const FunctionTemplateDecl *ft = fd->getPrimaryTemplate())
    for (const NamedDecl *nd : llvm::reverse(ft->getTemplateParameters()->asArray()))
      if (auto *ttpd = dyn_cast<TemplateTypeParmDecl>(nd); ttpd && ttpd->isParameterPack())
#if LLVM_VERSION_MAJOR >= 16
        return ttpd;
#else
        return cast<TemplateTypeParmType>(ttpd->getTypeForDecl());
#endif
  return nullptr;
}

// Find the first call in a function body that forwards the expanded pack
// params, e.g. `T(std::forward<Args>(args)...)`, and the callee parameters
// bound to them.
struct ForwardingCallVisitor : RecursiveASTVisitor<ForwardingCallVisitor> {
  ArrayRef<const ParmVarDecl *> params;
  // The bound parameters before, in and after the callee's own expanded pack.
  ArrayRef<const ParmVarDecl *> head, pack, tail;
  // The callee if it has an expanded pack.
  const FunctionDecl *pack_target = nullptr;
  bool found = false;

  ForwardingCallVisitor(ArrayRef<const ParmVarDecl *> params) : params(params) {}

  bool VisitCallExpr(CallExpr *e) {
    auto *callee = dyn_cast_or_null<FunctionDecl>(e->getCalleeDecl());
    if (callee && callee->getNumParams() == e->getNumArgs())
      handleCall(callee, {e->getArgs(), e->getNumArgs()});
    return !found;
  }

  // A copy or move is not a forwarding target, e.g. passing a pack element to a
  // by-value parameter of a functor.
  bool VisitCXXConstructExpr(CXXConstructExpr *e) {
    if (const CXXConstructorDecl *ctor = e->getConstructor(); ctor && !ctor->isCopyOrMoveConstructor())
      handleCall(ctor, {e->getArgs(), e->getNumArgs()});
    return !found;
  }

  void handleCall(const FunctionDecl *callee, ArrayRef<const Expr *> args) {
    if (callee->getNumParams() < params.size() || args.size() < params.size() ||
        llvm::any_of(args, [](const Expr *e) { return isa<PackExpansionExpr>(e); }))
      return;
    std::optional<size_t> pos = findPack(args);
    // Part of the pack may be passed to the `...` of a C variadic function.
    if (!pos || callee->getNumParams() < *pos + params.size())
      return;
    found = true;
    auto matching = ArrayRef<const ParmVarDecl *>(callee->parameters()).slice(*pos, params.size());
    head = matching;
    if (PackId id = getFunctionPack(callee)) {
      auto is_expanded = [&](const ParmVarDecl *p) { return getUnderlyingPack(p) == id; };
      head = matching.take_until(is_expanded);
      pack = matching.drop_front(head.size()).take_while(is_expanded);
      tail = matching.drop_front(head.size() + pack.size());
      pack_target = callee;
    }
  }

  std::optional<size_t> findPack(ArrayRef<const Expr *> args) {
    for (size_t i = 0; i + params.size() <= args.size(); i++)
      if (getForwardedDecl(args[i]) == params.front() && getForwardedDecl(args[i + params.size() - 1]) == params.back())
        return i;
    return std::nullopt;
  }

  // Look through std::forward and an implicit copy or move.
  static const ValueDecl *getForwardedDecl(const Expr *e) {
    e = e->IgnoreImplicitAsWritten();
    if (auto *ce = dyn_cast<CXXConstructExpr>(e))
      if (ce->getConstructor()->isCopyOrMoveConstructor())
        e = ce->getArg(0)->IgnoreImplicitAsWritten();
    if (auto *call = dyn_cast<CallExpr>(e))
      if (const FunctionDecl *fd = call->getDirectCallee();
          fd && call->getNumArgs() == 1 && fd->isInStdNamespace() && fd->getIdentifier() && fd->getName() == "forward")
        e = call->getArg(0)->IgnoreImplicitAsWritten();
    auto *dre = dyn_cast<DeclRefExpr>(e);
    return dre ? dre->getDecl() : nullptr;
  }
};

// Map each parameter of fd to the parameter it is eventually forwarded to, e.g.
// the constructor parameters for the arguments of `std::make_unique<T>(...)`.
SmallVector<const ParmVarDecl *, 8> resolveForwardingParameters(const FunctionDecl *fd) {
  ArrayRef<const ParmVarDecl *> params = fd->parameters();
  SmallVector<const ParmVarDecl *, 8> ret(params.begin(), params.end());
  PackId id = getFunctionPack(fd);
  if (!id)
    return ret;
  auto is_expanded = [&](const ParmVarDecl *p) { return getUnderlyingPack(p) == id; };
  ArrayRef<const ParmVarDecl *> pack = params.drop_until(is_expanded).take_while(is_expanded);
  // [begin, end) of ret is mapped to the unresolved pack.
  size_t begin = pack.data() - params.data(), end = begin + pack.size();
  llvm::SmallPtrSet<const FunctionTemplateDecl *, 4> seen{fd->getPrimaryTemplate()};
  const FunctionDecl *cur = fd;
  for (unsigned depth = 0; pack.size() && cur && depth < ForwardingToConstructorVisitor::kMaxDepth; depth++) {
    ForwardingCallVisitor visitor(pack);
    // e.g. std::shared_ptr's constructor forwards to its base in an initializer.
    if (auto *ctor = dyn_cast<CXXConstructorDecl>(cur))
      for (const CXXCtorInitializer *init : ctor->inits())
        if (init->isWritten() && !visitor.found)
          visitor.TraverseStmt(init->getInit());
    if (!visitor.found)
      visitor.TraverseStmt(cur->getBody());
    if (!visitor.found)
      break;
    llvm::copy(visitor.head, ret.begin() + begin);
    begin += visitor.head.size();
    end -= visitor.tail.size();
    llvm::copy(visitor.tail, ret.begin() + end);
    pack = visitor.pack;
    cur = visitor.pack_target;
    if (cur && !seen.insert(cur->getPrimaryTemplate()).second)
      return {params.begin(), params.end()};
  }
  llvm::copy(pack, ret.begin() + begin);
  return ret;
}

StringRef getSimpleName(DeclarationName name) {
  IdentifierInfo *ii = name.getAsIdentifierInfo();
  return ii ? ii->getName() : "";
}

StringRef getSimpleName(const NamedDecl *d) { return getSimpleName(d->getDeclName()); }

// Returns true if name is reserved, like _Foo or __Vector_base.
bool isReservedName(StringRef name) {
  return name.size() >= 2 && name[0] == '_' && (isUppercase(name[1]) || name[1] == '_');
}

// Collect the designators of the initializers in the semantic init list sem,
// descending into subobjects whose braces are elided, e.g. `.a` and `.b.x` for
// `Outer o{{1, 2}, 3}` where Outer has members `Inner a, b`.
void collectDesignators(const InitListExpr *sem, llvm::DenseMap<SourceLocation, std::string> &out,
                        std::string &prefix) {
  if (!sem || sem->isTransparent() || sem->getType().isNull())
    return;
  QualType t = sem->getType().getCanonicalType();
  bool is_array = t->isArrayType();
  const RecordDecl *rd = is_array ? nullptr : t->getAsRecordDecl();
  if (!is_array && !rd)
    return;
  unsigned num_bases = 0;
  if (auto *crd = dyn_cast_or_null<CXXRecordDecl>(rd)) {
    if (!crd->isAggregate())
      return;
    num_bases = crd->getNumBases();
  }
  RecordDecl::field_iterator field, field_end;
  if (rd)
    field = rd->field_begin(), field_end = rd->field_end();
  // e.g. std::array { T _M_elems[N]; }
  bool one_field = rd && !num_bases && field != field_end && std::next(field) == field_end;
  unsigned index = 0;
  for (const Expr *init : sem->inits()) {
    unsigned i = index++;
    const FieldDecl *fd = nullptr;
    if (rd) {
      // Bases cannot be designated.
      if (i < num_bases)
        continue;
      // Unnamed bit-fields have no initializers.
      while (field != field_end && field->isBitField() && !field->getIdentifier())
        ++field;
      if (field == field_end)
        break;
      fd = *field++;
    }
    if (!init || isa<ImplicitValueInitExpr>(init))
      continue;
    auto *elided = dyn_cast<InitListExpr>(init);
    if (elided && elided->isExplicit())
      elided = nullptr;
    size_t size = prefix.size();
    if (!fd) {
      prefix += "[" + std::to_string(i) + "]";
    } else {
      StringRef name = getSimpleName(fd);
      // Members of an anonymous struct/union or std::array can be named
      // directly.
      if (!elided || !(fd->isAnonymousStructOrUnion() || (one_field && isReservedName(name)))) {
        if (name.empty() || isReservedName(name))
          continue;
        prefix += ("." + name).str();
      }
    }
    if (elided)
      collectDesignators(elided, out, prefix);
    else
      out.try_emplace(init->getBeginLoc(), prefix);
    prefix.resize(size);
  }
}

// Returns a short form of an expression, or "" if it is too complex, e.g.
// "bar()" for `foo->bar()`.
std::string summarizeExpr(const Expr *e, const PrintingPolicy &pp) {
  struct Namer : ConstStmtVisitor<Namer, std::string> {
    const PrintingPolicy &pp;
    bool inside_binary = false;

    Namer(const PrintingPolicy &pp) : pp(pp) {}

    static std::string name(const NamedDecl *d) { return getSimpleName(d).str(); }
    std::string name(QualType t) {
      if (auto *bt = dyn_cast<BuiltinType>(t.getTypePtr()))
        return bt->getName(pp).str();
      if (const TagDecl *td = t->getAsTagDecl())
        return name(td);
      return "";
    }

    std::string Visit(const Expr *e) { return e ? ConstStmtVisitor::Visit(e->IgnoreImplicit()) : ""; }
    std::string VisitMemberExpr(const MemberExpr *e) { return name(e->getMemberDecl()); }
    std::string VisitDeclRefExpr(const DeclRefExpr *e) { return name(e->getFoundDecl()); }
    std::string VisitCallExpr(const CallExpr *e) {
      return Visit(e->getCallee()) + (e->getNumArgs() == 0 ? "()" : "(...)");
    }
    std::string VisitCXXDependentScopeMemberExpr(const CXXDependentScopeMemberExpr *e) {
      return getSimpleName(e->getMember()).str();
    }
    std::string VisitDependentScopeDeclRefExpr(const DependentScopeDeclRefExpr *e) {
      return getSimpleName(e->getDeclName()).str();
    }
    std::string VisitCXXFunctionalCastExpr(const CXXFunctionalCastExpr *e) { return name(e->getType()); }
    std::string VisitCXXTemporaryObjectExpr(const CXXTemporaryObjectExpr *e) { return name(e->getType()); }
    std::string VisitCXXMemberCallExpr(const CXXMemberCallExpr *e) {
      // `operator bool()` called in `if (x)`
      if (e->getNumArgs() == 0 && e->getMethodDecl() &&
          e->getMethodDecl()->getDeclName().getNameKind() == DeclarationName::CXXConversionFunctionName &&
          e->getSourceRange() == e->getImplicitObjectArgument()->getSourceRange())
        return Visit(e->getImplicitObjectArgument());
      return VisitCallExpr(e);
    }
    std::string VisitCXXConstructExpr(const CXXConstructExpr *e) {
      return e->getNumArgs() == 1 ? Visit(e->getArg(0)) : "";
    }

    std::string VisitCXXNullPtrLiteralExpr(const CXXNullPtrLiteralExpr *) { return "nullptr"; }
    std::string VisitCXXBoolLiteralExpr(const CXXBoolLiteralExpr *e) { return e->getValue() ? "true" : "false"; }
    std::string VisitIntegerLiteral(const IntegerLiteral *e) {
      std::string ret;
      llvm::raw_string_ostream os(ret);
      e->getValue().print(os, e->getType()->isSignedIntegerType());
      return os.str();
    }
    std::string VisitFloatingLiteral(const FloatingLiteral *e) {
      std::string ret;
      llvm::raw_string_ostream os(ret);
      e->getValue().print(os);
      os.flush();
      ret.resize(StringRef(ret).rtrim().size());
      return ret;
    }
    std::string VisitStringLiteral(const StringLiteral *e) {
      std::string ret = "\"";
      if (e->getCharByteWidth() != 1 || e->containsNonAscii()) {
        ret += "...";
      } else {
        llvm::raw_string_ostream os(ret);
        llvm::printEscapedString(e->getString().take_front(e->getLength() > 10 ? 7 : 10), os);
        os.flush();
        if (e->getLength() > 10)
          ret += "...";
      }
      return ret + "\"";
    }

    std::string printUnary(StringRef spelling, const Expr *operand, bool prefix) {
      std::string sub = Visit(operand);
      if (sub.empty())
        return "";
      return prefix ? (spelling + sub).str() : sub + spelling.str();
    }
    std::string printBinary(StringRef spelling, const Expr *lhs_op, const Expr *rhs_op) {
      if (inside_binary)
        return "";
      llvm::SaveAndRestore save(inside_binary, true);
      std::string lhs = Visit(lhs_op), rhs = Visit(rhs_op);
      if (lhs.empty() && rhs.empty())
        return "";
      return (lhs.empty() ? "..." : lhs) + " " + spelling.str() + " " + (rhs.empty() ? "..." : rhs);
    }
    std::string VisitUnaryOperator(const UnaryOperator *e) {
      return printUnary(UnaryOperator::getOpcodeStr(e->getOpcode()), e->getSubExpr(), !e->isPostfix());
    }
    std::string VisitBinaryOperator(const BinaryOperator *e) {
      return printBinary(BinaryOperator::getOpcodeStr(e->getOpcode()), e->getLHS(), e->getRHS());
    }
    std::string VisitCXXOperatorCallExpr(const CXXOperatorCallExpr *e) {
      const char *spelling = getOperatorSpelling(e->getOperator());
      if ((e->getOperator() == OO_PlusPlus || e->getOperator() == OO_MinusMinus) && e->getNumArgs() == 2)
        return printUnary(spelling, e->getArg(0), false);
      if (e->isInfixBinaryOp())
        return printBinary(spelling, e->getArg(0), e->getArg(1));
      if (e->getNumArgs() == 1)
        switch (e->getOperator()) {
        case OO_Plus:
        case OO_Minus:
        case OO_Star:
        case OO_Amp:
        case OO_Tilde:
        case OO_Exclaim:
        case OO_PlusPlus:
        case OO_MinusMinus:
          return printUnary(spelling, e->getArg(0), true);
        default:
          break;
        }
      return "";
    }
  };
  return Namer(pp).Visit(e);
}

// Return the TypeLoc of the invented template parameter if tl is written with
// `auto`, e.g. `const auto &` in `void f(const auto &x)`.
TemplateTypeParmTypeLoc getContainedAutoParamType(TypeLoc tl) {
  if (auto qtl = tl.getAs<QualifiedTypeLoc>())
    return getContainedAutoParamType(qtl.getUnqualifiedLoc());
  if (isa<PointerType>(tl.getTypePtr()) || isa<ReferenceType>(tl.getTypePtr()) || isa<ParenType>(tl.getTypePtr()))
    return getContainedAutoParamType(tl.getNextTypeLoc());
  if (auto ftl = tl.getAs<FunctionTypeLoc>())
    return getContainedAutoParamType(ftl.getReturnLoc());
  if (auto ttptl = tl.getAs<TemplateTypeParmTypeLoc>())
    if (ttptl.getTypePtr()->getDecl() && ttptl.getTypePtr()->getDecl()->isImplicit())
      return ttptl;
  return {};
}

const ParmVarDecl *getOnlyInstantiatedParam(const ParmVarDecl *pvd) {
  auto *fd = dyn_cast<FunctionDecl>(pvd->getDeclContext());
  const FunctionDecl *only = getOnlyInstantiation(fd);
  if (!only)
    return nullptr;
  unsigned i = 0;
  for (const ParmVarDecl *p : fd->parameters()) {
    // A preceding pack may expand to any number of parameters.
    if (p->isParameterPack())
      return nullptr;
    if (p == pvd)
      break;
    i++;
  }
  return i < only->getNumParams() ? only->getParamDecl(i) : nullptr;
}

#if LLVM_VERSION_MAJOR >= 14 // llvmorg-14-init-11692-gec64d10340da
// Whether desugaring t steps through a substituted template type parameter,
// e.g. `std::vector<int>::value_type`.
bool isSugaredTemplateParameter(QualType t) {
  while (true) {
    if (t->getAs<SubstTemplateTypeParmType>())
      return true;
    QualType desugared = t->getLocallyUnqualifiedSingleStepDesugaredType();
    if (desugared != t)
      t = desugared;
    else if (QualType pointee = desugared->getPointeeType(); !pointee.isNull() && pointee != t)
      t = pointee;
    else
      return false;
  }
}
#endif

QualType maybeDesugar(ASTContext &ctx, QualType t) {
#if LLVM_VERSION_MAJOR >= 14 // llvmorg-14-init-11692-gec64d10340da
  if (isSugaredTemplateParameter(t)) {
    bool should_aka = false;
    QualType desugared = desugarForDiagnostic(ctx, t, should_aka);
    return should_aka ? desugared : t;
  }
#endif
  if (isa<DecltypeType>(t.getTypePtr()))
    return t.getCanonicalType();
  if (const AutoType *at = t->getContainedAutoType())
    if (!at->getDeducedType().isNull() && isa<DecltypeType>(at->getDeducedType().getTypePtr()))
      return t.getCanonicalType();
  return t;
}

class InlayHintVisitor : public RecursiveASTVisitor<InlayHintVisitor> {
  ASTContext &ctx;
  const SourceManager &sm;
  IndexParam &param;
  PrintingPolicy type_policy;
#if LLVM_VERSION_MAJOR >= 22 // llvmorg-22-init-7955-ge07af8cbbfa2
  HeuristicResolver resolver;
#endif
  llvm::DenseSet<const IfStmt *> else_ifs;
  llvm::DenseSet<const InitListExpr *> std_init_lists;

public:
  InlayHintVisitor(ASTContext &ctx, IndexParam &param)
      : ctx(ctx), sm(ctx.getSourceManager()), param(param), type_policy(ctx.getPrintingPolicy())
#if LLVM_VERSION_MAJOR >= 22
        ,
        resolver(ctx)
#endif
  {
    type_policy.SuppressScope = true;
    setPlainAnonymousTag(type_policy);
  }

  // Skip declarations in system headers and in files not indexed by this
  // translation unit.
  bool TraverseDecl(Decl *d) {
    if (d && !isa<TranslationUnitDecl>(d) && !isa<NamespaceDecl>(d) && !isa<LinkageSpecDecl>(d)) {
      SourceLocation loc = sm.getExpansionLoc(d->getLocation());
      FileID fid = sm.getFileID(loc);
      if (fid.isInvalid() || sm.isInSystemHeader(loc) || !param.consumeFile(fid))
        return true;
    }
    return RecursiveASTVisitor::TraverseDecl(d);
  }

  bool TraversePseudoObjectExpr(PseudoObjectExpr *e) {
    Expr *syntactic = e->getSyntacticForm();
    // The semantic forms share the locations of the syntactic form, e.g.
    // __builtin_dump_struct.
    if (isa<CallExpr>(syntactic))
      return TraverseStmt(syntactic);
    // MS property `x = y` is a call to a setter.
    if (isa<BinaryOperator>(syntactic))
      return true;
    return RecursiveASTVisitor::TraversePseudoObjectExpr(e);
  }

  bool VisitTypeLoc(TypeLoc tl) {
    if (auto *dt = dyn_cast<DecltypeType>(tl.getTypePtr()))
      if (QualType ut = dt->getUnderlyingType(); !ut->isDependentType())
        addTypeHint(tl.getSourceRange(), ut, ": ");
    return true;
  }

  bool VisitCallExpr(CallExpr *e) {
    bool functor = false;
    if (auto *oce = dyn_cast<CXXOperatorCallExpr>(e)) {
      if (oce->getOperator() != OO_Call)
        return true;
      functor = true;
    }
    if (isa<UserDefinedLiteral>(e))
      return true;

    const FunctionDecl *callee = nullptr;
#if LLVM_VERSION_MAJOR >= 22
    auto callees = resolver.resolveCalleeOfCallExpr(e);
    if (callees.size() != 1)
      return true;
    if (auto *ftd = dyn_cast<FunctionTemplateDecl>(callees[0]))
      callee = ftd->getTemplatedDecl();
    else
      callee = dyn_cast<FunctionDecl>(callees[0]);
#else
    callee = dyn_cast_or_null<FunctionDecl>(e->getCalleeDecl());
#endif
    ArrayRef<ParmVarDecl *> params;
    if (callee)
      params = callee->parameters();
#if LLVM_VERSION_MAJOR >= 22
    else if (FunctionProtoTypeLoc proto = resolver.getFunctionProtoTypeLoc(e->getCallee()))
      params = proto.getParams();
#endif
    else
      return true;

    // The implied object argument precedes the arguments of a functor call or
    // a call to an explicit object member function.
    ArrayRef<const Expr *> args(e->getArgs(), e->getNumArgs());
    if (auto *md = dyn_cast_or_null<CXXMethodDecl>(callee)) {
      bool drop = functor;
#if LLVM_VERSION_MAJOR >= 18 // llvmorg-18-init-7480-gaf4751738db8
      drop |= !e->isTypeDependent() && md->hasCXXExplicitFunctionObjectParameter();
#endif
      if (drop)
        args = args.drop_front();
    }
    processCall(callee, params, args);
    return true;
  }

  bool VisitCXXConstructExpr(CXXConstructExpr *e) {
    if (e->getParenOrBraceRange().isInvalid() || e->isStdInitListInitialization())
      return true;
    if (const CXXConstructorDecl *ctor = e->getConstructor())
      processCall(ctor, ctor->parameters(), {e->getArgs(), e->getNumArgs()});
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *d) {
    if (auto *fpt = dyn_cast<FunctionProtoType>(d->getType().getTypePtr()))
      if (!fpt->hasTrailingReturn())
        if (FunctionTypeLoc ftl = d->getFunctionTypeLoc())
          addReturnTypeHint(d, ftl.getLocalRangeEnd());
    if (d->isThisDeclarationADefinition())
      if (Stmt *body = d->getBody())
        addBlockEndHint(body->getSourceRange(), "", [&] { return printFunctionName(d); });
    return true;
  }

  bool VisitForStmt(ForStmt *s) {
    addBlockEndHint(s->getBody(), "for", [&] {
      // Use the loop variable in `for (int i = 0; i < n; i++)`.
      if (auto *ds = dyn_cast_or_null<DeclStmt>(s->getInit()); ds && ds->isSingleDecl())
        return getSimpleName(cast<NamedDecl>(ds->getSingleDecl())).str();
      return summarizeExpr(s->getCond(), type_policy);
    });
    return true;
  }

  bool VisitCXXForRangeStmt(CXXForRangeStmt *s) {
    addBlockEndHint(s->getBody(), "for", [&] { return getSimpleName(s->getLoopVariable()).str(); });
    return true;
  }

  bool VisitWhileStmt(WhileStmt *s) {
    addBlockEndHint(s->getBody(), "while", [&] { return summarizeExpr(s->getCond(), type_policy); });
    return true;
  }

  bool VisitSwitchStmt(SwitchStmt *s) {
    addBlockEndHint(s->getBody(), "switch", [&] { return summarizeExpr(s->getCond(), type_policy); });
    return true;
  }

  bool VisitIfStmt(IfStmt *s) {
    // An else-if ends the whole chain; don't label it with its own condition.
    if (auto *else_if = dyn_cast_or_null<IfStmt>(s->getElse()))
      else_ifs.insert(else_if);
    if (auto *end = dyn_cast<CompoundStmt>(s->getElse() ? s->getElse() : s->getThen()))
      addBlockEndHint({s->getThen()->getBeginLoc(), end->getRBracLoc()}, "if",
                      [&] { return else_ifs.contains(s) ? "" : summarizeExpr(s->getCond(), type_policy); });
    return true;
  }

  bool VisitTagDecl(TagDecl *d) {
    if (d->isThisDeclarationADefinition()) {
      std::string prefix = d->getKindName().str();
      if (auto *ed = dyn_cast<EnumDecl>(d); ed && ed->isScoped())
        prefix += ed->isScopedUsingClassTag() ? " class" : " struct";
      addBlockEndHint(d->getBraceRange(), prefix, [&] { return getSimpleName(d).str(); }, ";");
    }
    return true;
  }

  bool VisitNamespaceDecl(NamespaceDecl *d) {
    std::string name = getSimpleName(d).str();
#if LLVM_VERSION_MAJOR >= 16 // llvmorg-16-init-11611-g15e76eed0c76
    // Label `namespace a::b {}` once, at the innermost declaration.
    if (d->decls_begin() != d->decls_end())
      if (auto *nd = dyn_cast<NamespaceDecl>(*d->decls_begin()); nd && nd->isNested())
        return true;
    for (const NamespaceDecl *nd = d; nd->isNested();) {
      nd = cast<NamespaceDecl>(nd->getParent());
      name = (getSimpleName(nd) + "::" + name).str();
    }
#endif
    addBlockEndHint(d->getSourceRange(), "namespace", [&] { return name; });
    return true;
  }

  // The elements of a std::initializer_list are not designated.
  bool VisitCXXStdInitializerListExpr(CXXStdInitializerListExpr *e) {
    if (auto *ile = dyn_cast<InitListExpr>(e->getSubExpr()->IgnoreImplicit()))
      std_init_lists.insert(ile->getSyntacticForm() ? ile->getSyntacticForm() : ile);
    return true;
  }

  bool VisitInitListExpr(InitListExpr *syn) {
    if (syn->isIdiomaticZeroInitializer(ctx.getLangOpts()) || std_init_lists.contains(syn))
      return true;
    llvm::DenseMap<SourceLocation, std::string> designators;
    std::string prefix;
    collectDesignators(syn->isSemanticForm() ? syn : syn->getSemanticForm(), designators, prefix);
    for (const Expr *init : syn->inits()) {
      if (isa<DesignatedInitExpr>(init))
        continue;
      auto it = designators.find(init->getBeginLoc());
      if (it != designators.end() && !isPrecededByParamNameComment(init, it->second))
        addHint(init->getSourceRange(), InlayHintKind::Designator, it->second + "=");
    }
    return true;
  }

#if LLVM_VERSION_MAJOR >= 16 // llvmorg-16-init-17081-g95a4c0c83554
  bool VisitCXXParenListInitExpr(CXXParenListInitExpr *e) {
    const CXXRecordDecl *rd = e->getType()->getAsCXXRecordDecl();
    if (!rd)
      return true;
    auto inits = e->getUserSpecifiedInitExprs();
    auto it = inits.begin() + std::min<size_t>(inits.size(), rd->getNumBases());
    for (const FieldDecl *field : rd->fields()) {
      if (it == inits.end())
        break;
      if (field->isBitField() && !field->getIdentifier())
        continue;
      const Expr *init = *it++;
      if (field->getIdentifier())
        addHint(init->getSourceRange(), InlayHintKind::Designator, ("." + field->getName() + "=").str());
    }
    return true;
  }
#endif

  bool VisitLambdaExpr(LambdaExpr *e) {
    FunctionDecl *d = e->getCallOperator();
    if (!e->hasExplicitResultType()) {
      SourceLocation loc;
      if (FunctionTypeLoc ftl = d->getFunctionTypeLoc())
        loc = ftl.getLocalRangeEnd();
      else if (!e->hasExplicitParameters())
        loc = e->getIntroducerRange().getEnd();
      if (loc.isValid())
        addReturnTypeHint(d, loc);
    }
    return true;
  }

  bool VisitVarDecl(VarDecl *d) {
    if (auto *dd = dyn_cast<DecompositionDecl>(d)) {
      // The canonical type avoids `tuple_element<I, A>::type`.
      for (BindingDecl *b : dd->bindings())
        if (QualType t = b->getType(); !t.isNull() && !t->isDependentType())
          addTypeHint(b->getLocation(), t.getCanonicalType(), ": ");
      return true;
    }

    if (const AutoType *at = d->getType()->getContainedAutoType(); at && at->isDeduced()) {
      QualType t = d->getType();
      if (t->isDependentType()) {
        t = {};
        if (const VarDecl *vd1 = getOnlyInstantiatedVar(d))
          t = vd1->getType();
#if LLVM_VERSION_MAJOR >= 22
        else if (d->hasInit())
          if (QualType resolved = resolver.resolveExprToType(d->getInit()); resolved != ctx.DependentTy)
            t = resolved;
#endif
      }
      addTypeHint(d->getLocation(), t, ": ");
    }

    if (auto *pvd = dyn_cast<ParmVarDecl>(d))
      if (pvd->getIdentifier() && pvd->getType()->isDependentType() && pvd->getTypeSourceInfo() &&
          !getContainedAutoParamType(pvd->getTypeSourceInfo()->getTypeLoc()).isNull())
        if (const ParmVarDecl *pvd1 = getOnlyInstantiatedParam(pvd))
          addTypeHint(pvd->getLocation(), pvd1->getType(), ": ");
    return true;
  }

private:
  void processCall(const FunctionDecl *callee, ArrayRef<ParmVarDecl *> params, ArrayRef<const Expr *> args) {
    if (args.empty() || (callee && isSimpleStdFunction(callee)))
      return;
    // The parameter name of a copy or move constructor is uninteresting.
    if (auto *ctor = dyn_cast_or_null<CXXConstructorDecl>(callee))
      if (ctor->isCopyOrMoveConstructor())
        return;

    SmallVector<const ParmVarDecl *, 8> forwarded =
        callee ? resolveForwardingParameters(callee)
               : SmallVector<const ParmVarDecl *, 8>(params.begin(), params.end());
#if LLVM_VERSION_MAJOR >= 18 // llvmorg-18-init-7480-gaf4751738db8
    if (!params.empty() && params.front()->isExplicitObjectParameter()) {
      params = params.drop_front();
      forwarded.erase(forwarded.begin());
    }
#endif
    SmallVector<StringRef, 8> names;
    for (const ParmVarDecl *p : forwarded) {
      // `args: 1, args: 2` is unlikely to be useful.
      StringRef name;
      if (!isExpandedFromParameterPack(p)) {
        name = getParamName(p);
        // Standard library parameter names often start with underscores.
        name = name.ltrim('_');
      }
      names.push_back(name);
    }
    if (callee && isSetter(callee, names))
      return;

    for (size_t i = 0; i < names.size() && i < args.size(); i++) {
      // A pack expansion breaks the 1:1 mapping between arguments and
      // parameters.
      if (isa<PackExpansionExpr>(args[i]))
        break;
      if (isa<CXXDefaultArgExpr>(args[i]))
        continue;
      bool name_hint = shouldHintName(args[i], names[i]), ref_hint = shouldHintReference(params[i], forwarded[i]);
      if (name_hint || ref_hint)
        addHint(args[i]->getSourceRange(), InlayHintKind::Parameter,
                (Twine(ref_hint ? "&" : "") + (name_hint ? names[i] : "") + ":").str());
    }
  }

  // If p is unnamed, use the name from the definition.
  static StringRef getParamName(const ParmVarDecl *p) {
    if (IdentifierInfo *ii = p->getIdentifier())
      return ii->getName();
    if (auto *fd = dyn_cast<FunctionDecl>(p->getDeclContext()))
      if (const FunctionDecl *def = fd->getDefinition()) {
        auto it = llvm::find(fd->parameters(), p);
        if (it != fd->param_end())
          if (IdentifierInfo *ii = def->getParamDecl(it - fd->param_begin())->getIdentifier())
            return ii->getName();
      }
    return {};
  }

  // A function with one parameter whose name is "set" followed by the
  // parameter name, e.g. setTimeout(timeout).
  static bool isSetter(const FunctionDecl *callee, ArrayRef<StringRef> names) {
    if (names.size() != 1 || !callee->getIdentifier())
      return false;
    StringRef name = callee->getName();
    return name.size() > 3 && name.substr(0, 3).lower() == "set" &&
           name.substr(3).ltrim('_').lower() == names[0].lower();
  }

  static bool isSimpleStdFunction(const FunctionDecl *callee) {
    if (!callee->isInStdNamespace() || !callee->getIdentifier() || callee->getNumParams() != 1)
      return false;
    StringRef name = callee->getName();
    return name == "addressof" || name == "as_const" || name == "forward" || name == "move" ||
           name == "move_if_noexcept";
  }

  bool shouldHintName(const Expr *arg, StringRef name) {
    return name.size() && name != getSpelledIdentifier(arg) && !isPrecededByParamNameComment(arg, name);
  }

  // A forwarded argument is passed by mutable reference only if every function
  // in the chain takes an lvalue reference: a copy along the way would bind to
  // an rvalue or const reference.
  static bool shouldHintReference(const ParmVarDecl *p, const ParmVarDecl *forwarded) {
    QualType t = forwarded->getType();
    return p->getType()->isLValueReferenceType() && t->isLValueReferenceType() &&
           !t.getNonReferenceType().isConstQualified() && !isExpandedFromParameterPack(forwarded);
  }

  static StringRef getSpelledIdentifier(const Expr *arg) {
    arg = arg->IgnoreUnlessSpelledInSource();
    if (auto *dre = dyn_cast<DeclRefExpr>(arg))
      if (!dre->getQualifier() && dre->getDecl()->getIdentifier())
        return dre->getDecl()->getName();
    if (auto *me = dyn_cast<MemberExpr>(arg))
      if (!me->getQualifier() && me->isImplicitAccess() && me->getMemberDecl()->getIdentifier())
        return me->getMemberDecl()->getName();
    return {};
  }

  // Whether arg is preceded by a comment like /*name=*/.
  bool isPrecededByParamNameComment(const Expr *arg, StringRef name) {
    auto [fid, offset] = sm.getDecomposedLoc(sm.getFileLoc(arg->getBeginLoc()));
    bool invalid = false;
    StringRef prefix = sm.getBufferData(fid, &invalid);
    if (invalid)
      return false;
    prefix = prefix.substr(0, offset).rtrim();
    if (!prefix.consume_back("*/"))
      return false;
    StringRef ignore = " =.";
    prefix = prefix.rtrim(ignore);
    if (!prefix.consume_back(name.trim(ignore)))
      return false;
    return prefix.rtrim(ignore).endswith("/*");
  }

  void addReturnTypeHint(FunctionDecl *d, SourceRange r) {
    const AutoType *at = d->getReturnType()->getContainedAutoType();
    if (at && !at->getDeducedType().isNull())
      addTypeHint(r, d->getReturnType(), "-> ");
  }

  void addTypeHint(SourceRange r, QualType t, StringRef prefix) {
    if (t.isNull())
      return;
    // `(lambda)` tells nothing the initializer doesn't.
    if (const CXXRecordDecl *rd = t.getNonReferenceType()->getAsCXXRecordDecl(); rd && rd->isLambda())
      return;
    // Prefer the desugared type unless it is too long.
    QualType desugared = maybeDesugar(ctx, t);
    std::string name = desugared.getAsString(type_policy);
    if (desugared != t && !fitsTypeNameLimit(name))
      name = t.getAsString(type_policy);
    if (fitsTypeNameLimit(name))
      addHint(r, InlayHintKind::Type, (Twine(prefix) + name).str());
  }

  static bool fitsTypeNameLimit(StringRef name) {
    int limit = g_config->inlayHint.typeNameLimit;
    return limit <= 0 || name.size() < size_t(limit);
  }

  // Print the qualifier as written, e.g. `S::f` for an out-of-line definition.
  std::string printFunctionName(const FunctionDecl *d) {
    std::string ret;
    llvm::raw_string_ostream os(ret);
    if (NestedNameSpecifierLoc qualifier = d->getQualifierLoc())
      os << Lexer::getSourceText(CharSourceRange::getTokenRange(qualifier.getSourceRange()), sm, ctx.getLangOpts());
    PrintingPolicy pp(ctx.getLangOpts());
    pp.SuppressTemplateArgsInCXXConstructors = true;
    d->getDeclName().print(os, pp);
    return os.str();
  }

  void addBlockEndHint(const Stmt *body, StringRef prefix, llvm::function_ref<std::string()> name) {
    if (auto *cs = dyn_cast_or_null<CompoundStmt>(body))
      addBlockEndHint(cs->getSourceRange(), prefix, name);
  }

  // Attach `// prefix name` after the closing brace of a block spanning at least
  // 10 lines if nothing but `punct` follows the brace on its line.
  void addBlockEndHint(SourceRange braces, StringRef prefix, llvm::function_ref<std::string()> name,
                       StringRef punct = "") {
    auto [begin_fid, begin_offset] = sm.getDecomposedLoc(sm.getFileLoc(braces.getBegin()));
    auto [fid, offset] = sm.getDecomposedLoc(sm.getFileLoc(braces.getEnd()));
    if (begin_fid != fid)
      return;
    bool invalid = false;
    StringRef rest = sm.getBufferData(fid, &invalid).substr(offset).split('\n').first;
    if (invalid || !rest.startswith("}"))
      return;
    StringRef trailing = rest.drop_front().trim();
    if (trailing.size() && trailing != punct)
      return;
    if (sm.getLineNumber(fid, offset) < sm.getLineNumber(fid, begin_offset) + 9)
      return;
    std::string label = prefix.str(), suffix = name();
    if (label.size() && suffix.size())
      label += ' ';
    label += suffix;
    if (label.size() <= 60)
      addHintAt(sm.getComposedLoc(fid, trailing.empty() ? offset + 1 : trailing.end() - rest.data() + offset),
                InlayHintKind::BlockEnd, "// " + label);
  }

  // Attach a type hint to the right of r and other hints to the left. r must be
  // spelled exactly in a file.
  void addHint(SourceRange r, InlayHintKind kind, StringRef label) {
    CharSourceRange csr = Lexer::makeFileCharRange(CharSourceRange::getTokenRange(r), sm, ctx.getLangOpts());
    if (csr.isValid())
      addHintAt(kind == InlayHintKind::Type ? csr.getEnd() : csr.getBegin(), kind, label);
  }

  void addHintAt(SourceLocation loc, InlayHintKind kind, StringRef label) {
    if (sm.isInSystemHeader(loc))
      return;
    FileID fid;
    Range range = fromCharSourceRange(sm, ctx.getLangOpts(), CharSourceRange::getCharRange(loc, loc), &fid);
    if (IndexFile *db = param.consumeFile(fid))
      db->inlay_hints.push_back({range.start, kind, intern(label)});
  }
};

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
        while (prefix > 0 && p < e && (*p == ' ' || *p == '/' || *p == '*' || *p == '<' || *p == '!'))
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
    setPlainAnonymousTag(pp);
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
  void setName(const Decl *d, std::string_view short_name, std::string_view qualified, Def &def) {
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
             ((i && isAsciiIdentifierContinue(name[i - 1])) || isAsciiIdentifierContinue(name[i + short_name.size()])))
        i = name.find(short_name, i + short_name.size());
    if (i == std::string::npos) {
      // e.g. operator type-parameter-1
      i = 0;
      def.short_name_offset = 0;
      def.short_name_size = name.size();
    } else {
      if (short_name.empty() || (i >= 2 && name[i - 2] == ':')) {
        // Don't replace name with qualified name in ns::name Cls::*name
        def.short_name_offset = i;
      } else {
        name.replace(i, short_name.size(), qualified);
        def.short_name_offset = i + qualified.size() - short_name.size();
      }
      // name may be empty while short_name is not.
      def.short_name_size = name.empty() ? 0 : short_name.size();
    }
    for (int paren = 0; i; i--) {
      // Skip parentheses in "(anon struct)::name"
      if (name[i - 1] == ')')
        paren++;
      else if (name[i - 1] == '(')
        paren--;
      else if (!(paren > 0 || isAsciiIdentifierContinue(name[i - 1]) || name[i - 1] == ':'))
        break;
    }
    def.qual_name_offset = i;
    def.detailed_name = intern(name);
  }

  void setVarName(const Decl *d, std::string_view short_name, std::string_view qualified, IndexVar::Def &def,
                  QualType deduced_type = QualType()) {
    QualType t;
    const Expr *init = nullptr;
    bool deduced = false;
    if (auto *vd = dyn_cast<VarDecl>(d)) {
      t = deduced_type.isNull() ? vd->getType() : deduced_type;
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
    std::string aka;
    if (!t.isNull() && deduced) {
      SmallString<256> str;
      llvm::raw_svector_ostream os(str);
      PrintingPolicy pp = getDefaultPolicy();
      t.print(os, pp);
#if LLVM_VERSION_MAJOR >= 14 // llvmorg-14-init-11692-gec64d10340da
      // Like clangd, show the desugared type if it differs, e.g. for iterator.
      bool should_aka = false;
      QualType desugared = desugarForDiagnostic(*ctx, t, should_aka);
      if (should_aka) {
        PrintingPolicy pp1 = pp;
        pp1.FullyQualifiedName = true;
        if ((aka = desugared.getAsString(pp1)) == str)
          aka.clear();
      }
#endif
      if (str.size() && (str.back() != ' ' && str.back() != '*' && str.back() != '&'))
        str += ' ';
      def.qual_name_offset = str.size();
      def.short_name_offset = str.size() + qualified.size() - short_name.size();
      def.short_name_size = short_name.size();
      str += StringRef(qualified.data(), qualified.size());
      def.detailed_name = intern(str);
    } else {
      setName(d, short_name, qualified, def);
    }
    std::string hover;
    if (init) {
      SourceManager &sm = ctx->getSourceManager();
      const LangOptions &lang = ctx->getLangOpts();
      SourceRange sr = sm.getExpansionRange(init->getSourceRange()).getAsRange();
      SourceLocation l = d->getLocation();
      if (!l.isMacroID() && sm.isBeforeInTranslationUnit(l, sr.getBegin())) {
        StringRef buf = getSourceInRange(sm, lang, sr);
        Twine init = buf.count('\n') <= g_config->index.maxInitializerLines - 1
                         ? buf.size() && buf[0] == ':' ? Twine(" ", buf) : Twine(" = ", buf)
                         : Twine();
        hover = (def.detailed_name + init).str();
      }
    }
    if (hover.empty() && aka.size())
      hover = def.detailed_name;
    if (hover.size()) {
      if (def.storage == SC_Static && strncmp(def.detailed_name, "static ", 7))
        hover.insert(0, "static ");
      if (aka.size())
        hover += "\n// aka " + aka;
      def.hover = intern(hover);
    }
  }

  static int getFileLID(IndexFile *db, SourceManager &sm, FileID fid) {
    auto [it, inserted] = db->uid2lid_and_path.try_emplace(fid);
    if (inserted) {
#if LLVM_VERSION_MAJOR < 19
      const FileEntry *fe = sm.getFileEntryForID(fid);
#else
      OptionalFileEntryRef fe = sm.getFileEntryRefForID(fid);
#endif
      if (!fe) {
        it->second.first = -1;
        return -1;
      }
      it->second.first = db->uid2lid_and_path.size() - 1;
      it->second.second = pathFromFileEntry(*fe);
    }
    return it->second.first;
  }

  void addMacroUse(IndexFile *db, SourceManager &sm, Usr usr, Kind kind, SourceLocation sl) const {
    FileID fid = sm.getFileID(sl);
    int lid = getFileLID(db, sm, fid);
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
      if (!rd->isCompleteDefinition() || rd->isDependentType() || rd->isInvalidDecl() || !validateRecord(rd))
        offset = -1;
      for (FieldDecl *fd : rd->fields()) {
        int offset1 = offset < 0 ? -1 : int(offset + ctx->getFieldOffset(fd));
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
  bool handleDeclOccurrence(const Decl *d, index::SymbolRoleSet roles, ArrayRef<index::SymbolRelation> relations,
                            SourceLocation src_loc, ASTNodeInfo ast_node) override {
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
    Range loc;
    auto r = sm.isMacroArgExpansion(src_loc) ? CharSourceRange::getTokenRange(spell) : sm.getExpansionRange(src_loc);
    loc = fromCharSourceRange(sm, lang, r);
    fid = sm.getFileID(r.getBegin());
    if (fid.isInvalid())
      return true;
    int lid = -1;
    IndexFile *db;
    if (g_config->index.multiVersion && param.useMultiVersion(fid)) {
      db = param.consumeFile(sm.getMainFileID());
      if (!db)
        return true;
      param.seenFile(fid);
      if (!sm.isWrittenInMainFile(r.getBegin()))
        lid = getFileLID(db, sm, fid);
    } else {
      db = param.consumeFile(fid);
      if (!db)
        return true;
    }

    // spell, extent, comments use OrigD while most others use adjusted |D|.
    const Decl *origD = ast_node.OrigD;
    const DeclContext *sem_dc = origD->getDeclContext()->getRedeclContext();
    const DeclContext *lex_dc = ast_node.ContainerDC->getRedeclContext();
    {
      const NamespaceDecl *nd;
      while ((nd = dyn_cast<NamespaceDecl>(cast<Decl>(sem_dc))) && nd->isAnonymousNamespace())
        sem_dc = nd->getDeclContext()->getRedeclContext();
      while ((nd = dyn_cast<NamespaceDecl>(cast<Decl>(lex_dc))) && nd->isAnonymousNamespace())
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
          SourceRange sr = cast<FunctionDecl>(origD)->getNameInfo().getSourceRange();
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
    if (isInstantiatedLocal(d))
      return true;

    IndexParam::DeclInfo *info;
    Usr usr = getUsr(d, &info);

    auto do_def_decl = [&](auto *entity) {
      Use use{{loc, role}, lid};
      if (is_def) {
        SourceRange sr = origD->getSourceRange();
        entity->def.spell = {use, fromTokenRangeDefaulted(sm, lang, sr, fid, loc)};
        entity->def.parent_kind = SymbolKind::File;
        getKind(cast<Decl>(sem_dc), entity->def.parent_kind);
      } else if (is_decl) {
        SourceRange sr = origD->getSourceRange();
        entity->declarations.push_back({use, fromTokenRangeDefaulted(sm, lang, sr, fid, loc)});
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
        LOG_S(INFO) << "Unhandled " << int(d->getKind()) << " " << info->qualified << " in " << db->path << ":"
                    << (loc.start.line + 1) << ":" << (loc.start.column + 1);
      return true;
    case Kind::File:
      return true;
    case Kind::Func:
      func = &db->toFunc(usr);
      func->def.kind = ls_kind;
      // Mark as Role::Implicit to span one more column to the left/right.
      if (!is_def && !is_decl && (d->getKind() == Decl::CXXConstructor || d->getKind() == Decl::CXXConversion))
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
        bool dc_is_func = getKind(dc, ls_kind) == Kind::Func;
        if (dc_is_func)
          db->toFunc(getUsr(dc)).def.callees.push_back({loc, usr, Kind::Func, role});
        if (const auto *fd = dyn_cast<FunctionDecl>(origD);
            fd && fd->isTemplateInstantiation() && isLikelyForwardingFunction(fd))
          param.deferred_forward.push_back({db, loc, Role(role | Role::Implicit), lid, fd, dc_is_func ? dc : nullptr});
      }
      break;
    case Kind::Type:
      type = &db->toType(usr);
      type->def.kind = ls_kind;
      do_def_decl(type);
      if (spell != src_loc)
        addMacroUse(db, sm, usr, Kind::Type, spell);
      if ((is_def || type->def.detailed_name[0] == '\0') && info->short_name.size()) {
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
      if (is_def || is_decl) {
        const Decl *dc = cast<Decl>(sem_dc);
        Kind kind = getKind(dc, var->def.parent_kind);
        if (kind == Kind::Func)
          db->toFunc(getUsr(dc)).def.vars.push_back(usr);
        else if (kind == Kind::Type && !isa<RecordDecl>(sem_dc))
          db->toType(getUsr(dc)).def.vars.emplace_back(usr, -1);
        if (auto *vd = dyn_cast<ValueDecl>(d))
          setVarType(db, *var, usr, d, vd->getType());
        if (auto *vd = dyn_cast<VarDecl>(d);
            vd && is_def && vd->getType()->isDependentType() && vd->getType()->getContainedDeducedType())
          param.deferred_var.push_back({db, vd, usr});
      } else if (!var->def.spell && var->declarations.empty()) {
        // e.g. lambda parameter
        SourceLocation l = d->getLocation();
        if (sm.getFileID(l) == fid) {
          var->def.spell = {Use{{fromTokenRange(sm, lang, {l, l}), Role::Definition}, lid},
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
            if (const Decl *baseD = getAdjustedDecl(getTypeDecl(base.getType()))) {
              Usr usr1 = getUsr(baseD);
              type->def.bases.push_back(usr1);
              db->toType(usr1).derived.push_back(usr);
            }
      }
      [[fallthrough]];
    case Decl::Enum:
    case Decl::Record:
      if (auto *tag_d = dyn_cast<TagDecl>(d)) {
        if (type->def.detailed_name[0] == '\0' && info->short_name.empty()) {
          StringRef tag;
          switch (tag_d->getTagKind()) {
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
          if (TypedefNameDecl *td = tag_d->getTypedefNameForAnonDecl()) {
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
        if (is_def && !isa<EnumDecl>(d))
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
            llvm::PointerUnion<ClassTemplateDecl *, ClassTemplatePartialSpecializationDecl *> result =
                sd->getSpecializedTemplateOrPartial();
#if LLVM_VERSION_MAJOR >= 15 // llvmorg-15-init-10510-gaab5bd180a42
            if (auto *ctd = dyn_cast<ClassTemplateDecl *>(result))
              d1 = ctd;
            else
              d1 = cast<ClassTemplatePartialSpecializationDecl *>(result);
#else
            if (result.is<ClassTemplateDecl *>())
              d1 = result.get<ClassTemplateDecl *>();
            else
              d1 = result.get<ClassTemplatePartialSpecializationDecl *>();
#endif

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
              type1.uses.push_back({{fromTokenRange(sm, lang, {l1, l1}), Role::Reference}, lid});
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
            " = " + (val.isSigned() ? std::to_string(val.getSExtValue()) : std::to_string(val.getZExtValue()));
        var->def.hover = intern(var->def.detailed_name + init);
      }
      break;
    default:
      break;
    }
    return true;
  }

  void setVarType(IndexFile *db, IndexVar &var, Usr usr, const Decl *d, QualType t) {
    if (t.isNull())
      return;
    Usr usr1;
    if (auto *bt = t->getAs<BuiltinType>())
      usr1 = static_cast<Usr>(bt->getKind());
    else if (const Decl *d1 = getAdjustedDecl(getTypeDecl(t)))
      usr1 = getUsr(d1);
    else
      return;
    var.def.type = usr1;
    if (!isa<EnumConstantDecl>(d))
      db->toType(usr1).instances.push_back(usr);
  }

  void finish() override {
    for (const auto &dv : param.deferred_var) {
      const VarDecl *vd1 = getOnlyInstantiatedVar(dv.vd);
      if (!vd1)
        continue;
      IndexParam::DeclInfo *info;
      getUsr(dv.vd, &info);
      IndexVar &var = dv.db->toVar(dv.usr);
      var.def.detailed_name = var.def.hover = "";
      setVarName(dv.vd, info->short_name, info->qualified, var.def, vd1->getType());
      setVarType(dv.db, var, dv.usr, dv.vd, vd1->getType());
    }
    llvm::DenseMap<const FunctionDecl *, llvm::SmallVector<const CXXConstructorDecl *, 1>> ctors;
    for (const auto &df : param.deferred_forward) {
      auto [it, inserted] = ctors.try_emplace(df.callee);
      if (inserted)
        it->second = searchConstructorsInForwardingFunction(df.callee);
      for (const CXXConstructorDecl *cd : it->second) {
        Usr cusr = getUsr(cd);
        df.db->toFunc(cusr).uses.push_back({{df.loc, df.role}, df.lid});
        if (df.caller)
          df.db->toFunc(getUsr(df.caller)).def.callees.push_back({df.loc, cusr, Kind::Func, df.role});
      }
    }
    if (param.no_linkage) {
      InlayHintVisitor(*ctx, param).TraverseDecl(ctx->getTranslationUnitDecl());
      // Explicit instantiations may duplicate hints.
      for (auto &[_, file] : param.uid2file)
        if (file.db) {
          std::sort(file.db->inlay_hints.begin(), file.db->inlay_hints.end());
          file.db->inlay_hints.erase(std::unique(file.db->inlay_hints.begin(), file.db->inlay_hints.end()),
                                     file.db->inlay_hints.end());
        }
    }
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
  IndexPPCallbacks(SourceManager &sm, IndexParam &param) : sm(sm), param(param) {}
  void FileChanged(SourceLocation sl, FileChangeReason reason, SrcMgr::CharacteristicKind, FileID) override {
    if (reason == FileChangeReason::EnterFile)
      (void)param.consumeFile(sm.getFileID(sl));
  }
  void InclusionDirective(SourceLocation hashLoc, const Token &tok, StringRef included, bool isAngled,
                          CharSourceRange filenameRange,
#if LLVM_VERSION_MAJOR >= 16 // llvmorg-16-init-15080-g854c10f8d185
                          OptionalFileEntryRef fileRef,
#elif LLVM_VERSION_MAJOR >= 15 // llvmorg-15-init-7692-gd79ad2f1dbc2
                          llvm::Optional<FileEntryRef> fileRef,
#else
                          const FileEntry *file,
#endif
                          StringRef searchPath, StringRef relativePath, const clang::Module *suggestedModule,
#if LLVM_VERSION_MAJOR >= 19 // llvmorg-19-init-1720-gda95d926f6fc
                          bool moduleImported,
#endif
                          SrcMgr::CharacteristicKind fileType) override {
#if LLVM_VERSION_MAJOR >= 15 // llvmorg-15-init-7692-gd79ad2f1dbc2
    const FileEntry *file = fileRef ? &fileRef->getFileEntry() : nullptr;
#endif
    if (!file)
      return;
    auto spell = fromCharSourceRange(sm, param.ctx->getLangOpts(), filenameRange, nullptr);
    FileID fid = sm.getFileID(filenameRange.getBegin());
    if (IndexFile *db = param.consumeFile(fid)) {
#if LLVM_VERSION_MAJOR < 19
      std::string path = pathFromFileEntry(*file);
#else
      std::string path = pathFromFileEntry(*fileRef);
#endif
      if (path.size())
        db->includes.push_back({spell.start.line, intern(path)});
    }
  }
  void MacroDefined(const Token &tok, const MacroDirective *md) override {
    const LangOptions &lang = param.ctx->getLangOpts();
    SourceLocation sl = md->getLocation();
    FileID fid = sm.getFileID(sl);
    if (IndexFile *db = param.consumeFile(fid)) {
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
        var.def.hover = intern(buf.count('\n') <= g_config->index.maxInitializerLines - 1
                                   ? Twine("#define ", getSourceInRange(sm, lang, sr)).str()
                                   : Twine("#define ", name).str());
      }
    }
  }
  void MacroExpands(const Token &tok, const MacroDefinition &, SourceRange sr, const MacroArgs *) override {
    SourceLocation sl = sm.getSpellingLoc(sr.getBegin());
    FileID fid = sm.getFileID(sl);
    if (IndexFile *db = param.consumeFile(fid)) {
      IndexVar &var = db->toVar(getMacro(tok).second);
      var.uses.push_back({{fromTokenRange(sm, param.ctx->getLangOpts(), {sl, sl}, nullptr), Role::Dynamic}});
    }
  }
  void MacroUndefined(const Token &tok, const MacroDefinition &md, const MacroDirective *ud) override {
    if (ud) {
      SourceLocation sl = ud->getLocation();
      MacroExpands(tok, md, {sl, sl}, nullptr);
    }
  }
  void SourceRangeSkipped(SourceRange sr, SourceLocation) override {
    Range range = fromCharSourceRange(sm, param.ctx->getLangOpts(), CharSourceRange::getCharRange(sr));
    FileID fid = sm.getFileID(sr.getBegin());
    if (fid.isValid())
      if (IndexFile *db = param.consumeFile(fid))
        db->skipped_ranges.push_back(range);
  }
};

class IndexFrontendAction : public ASTFrontendAction {
  std::shared_ptr<IndexDataConsumer> dataConsumer;
  const index::IndexingOptions &indexOpts;
  IndexParam &param;

public:
  IndexFrontendAction(std::shared_ptr<IndexDataConsumer> dataConsumer, const index::IndexingOptions &indexOpts,
                      IndexParam &param)
      : dataConsumer(std::move(dataConsumer)), indexOpts(indexOpts), param(param) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci, StringRef inFile) override {
    class SkipProcessed : public ASTConsumer {
      IndexParam &param;
      const ASTContext *ctx = nullptr;

    public:
      SkipProcessed(IndexParam &param) : param(param) {}
      void Initialize(ASTContext &ctx) override { this->ctx = &ctx; }
      bool shouldSkipFunctionBody(Decl *d) override {
        const SourceManager &sm = ctx->getSourceManager();
        FileID fid = sm.getFileID(sm.getExpansionLoc(d->getLocation()));
        return !(g_config->index.multiVersion && param.useMultiVersion(fid)) && !param.consumeFile(fid);
      }
    };

    std::shared_ptr<Preprocessor> pp = ci.getPreprocessorPtr();
    pp->addPPCallbacks(std::make_unique<IndexPPCallbacks>(pp->getSourceManager(), param));
    std::vector<std::unique_ptr<ASTConsumer>> consumers;
    consumers.push_back(std::make_unique<SkipProcessed>(param));
    consumers.push_back(index::createIndexingASTConsumer(dataConsumer, indexOpts, std::move(pp)));
    return std::make_unique<MultiplexConsumer>(std::move(consumers));
  }
};

class IndexDiags : public DiagnosticConsumer {
public:
  llvm::SmallString<64> message;
  void HandleDiagnostic(DiagnosticsEngine::Level level, const clang::Diagnostic &info) override {
    DiagnosticConsumer::HandleDiagnostic(level, info);
    if (message.empty())
      info.FormatDiagnostic(message);
  }
};
} // namespace

const int IndexFile::kMajorVersion = 22;
const int IndexFile::kMinorVersion = 0;

IndexFile::IndexFile(const std::string &path, const std::string &contents, bool no_linkage)
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

std::string IndexFile::toString() { return ccls::serialize(SerializeFormat::Json, *this); }

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
  multiVersionMatcher = new GroupMatch(g_config->index.multiVersionWhitelist, g_config->index.multiVersionBlacklist);
}

IndexResult index(WorkingFiles *wfiles, VFS *vfs, const std::string &opt_wdir, const std::string &main,
                  const std::vector<const char *> &args,
                  const std::vector<std::pair<std::string, std::string>> &remapped, bool no_linkage, bool &ok) {
  ok = true;
  auto pch = std::make_shared<PCHContainerOperations>();
  llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs = llvm::vfs::getRealFileSystem();
  std::shared_ptr<CompilerInvocation> ci = buildCompilerInvocation(main, args, fs);
  // e.g. .s
  if (!ci)
    return {};
  ok = false;
  // -fparse-all-comments enables documentation in the indexer and in
  // code completion.
#if LLVM_VERSION_MAJOR >= 24 // llvmorg-24-init-12130-g6cd72b33224f
  ci->getLangOpts().CommentOpts.ParseAllComments = g_config->index.comments > 1;
  ci->getLangOpts().CommentOpts.RetainComments = true;
  ci->getLangOpts().CommentOpts.RetainCommentsFromSystemHeaders = true;
#elif LLVM_VERSION_MAJOR >= 18
  ci->getLangOpts().CommentOpts.ParseAllComments = g_config->index.comments > 1;
  ci->getLangOpts().RetainCommentsFromSystemHeaders = true;
#else
  ci->getLangOpts()->CommentOpts.ParseAllComments = g_config->index.comments > 1;
  ci->getLangOpts()->RetainCommentsFromSystemHeaders = true;
#endif
  std::string buf = wfiles->getContent(main);
  std::vector<std::unique_ptr<llvm::MemoryBuffer>> bufs;
  if (buf.size())
    for (auto &[filename, content] : remapped) {
      bufs.push_back(llvm::MemoryBuffer::getMemBuffer(content));
      ci->getPreprocessorOpts().addRemappedFile(filename, bufs.back().get());
    }

  IndexDiags dc;
#if LLVM_VERSION_MAJOR >= 21
  auto clang = std::make_unique<CompilerInstance>(std::move(ci), pch);
#else
  auto clang = std::make_unique<CompilerInstance>(pch);
  clang->setInvocation(std::move(ci));
#endif
#if LLVM_VERSION_MAJOR >= 22
  // Must precede createDiagnostics, which dereferences the VFS. Use
  // createVirtualFileSystem (not setVirtualFileSystem) so -ivfsoverlay applies.
  clang->createVirtualFileSystem(fs);
#endif
  clang->createDiagnostics(
#if LLVM_VERSION_MAJOR >= 20 && LLVM_VERSION_MAJOR < 22
      *fs,
#endif
      &dc, false);
  clang->getDiagnostics().setIgnoreAllWarnings(true);
#if LLVM_VERSION_MAJOR >= 21
  clang->setTarget(TargetInfo::CreateTargetInfo(clang->getDiagnostics(), clang->getTargetOpts()));
#else
  clang->setTarget(TargetInfo::CreateTargetInfo(clang->getDiagnostics(), clang->getInvocation().TargetOpts));
#endif
  if (!clang->hasTarget())
    return {};
  clang->getPreprocessorOpts().RetainRemappedFileBuffers = true;
#if LLVM_VERSION_MAJOR >= 22
  clang->createFileManager();
#else
  clang->createFileManager(fs);
#endif
  clang->setSourceManager(new SourceManager(clang->getDiagnostics(), clang->getFileManager(), true));

  IndexParam param(*vfs, no_linkage);

  index::IndexingOptions indexOpts;
  indexOpts.SystemSymbolFilter = index::IndexingOptions::SystemSymbolFilterKind::All;
  if (no_linkage) {
    indexOpts.IndexFunctionLocals = true;
    indexOpts.IndexImplicitInstantiation = true;
    indexOpts.IndexParametersInDeclarations = g_config->index.parametersInDeclarations;
    indexOpts.IndexTemplateParameters = true;
  }

  auto action = std::make_unique<IndexFrontendAction>(std::make_shared<IndexDataConsumer>(param), indexOpts, param);
  std::string reason;
  {
    llvm::CrashRecoveryContext crc;
    auto parse = [&]() {
      if (!action->BeginSourceFile(*clang, clang->getFrontendOpts().Inputs[0]))
        return;
      if (llvm::Error e = action->Execute()) {
        reason = llvm::toString(std::move(e));
        return;
      }
      action->EndSourceFile();
      ok = true;
    };
    if (!crc.RunSafely(parse)) {
      LOG_S(ERROR) << "clang crashed for " << main;
      return {};
    }
  }
  if (!ok) {
    LOG_S(ERROR) << "failed to index " << main << (reason.empty() ? "" : ": " + reason);
    return {};
  }

  IndexResult result;
  result.n_errs = (int)dc.getNumErrors();
  // clang 7 does not implement operator std::string.
  result.first_error = std::string(dc.message.data(), dc.message.size());
  for (auto &it : param.uid2file) {
    if (!it.second.db)
      continue;
    std::unique_ptr<IndexFile> &entry = it.second.db;
    entry->import_file = main;
    entry->args = args;
    for (auto &[_, it] : entry->uid2lid_and_path)
      if (it.first >= 0)
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
      if (path.empty())
        continue;
      if (path == entry->path)
        entry->mtime = file.mtime;
      else if (path != entry->import_file)
        entry->dependencies[llvm::CachedHashStringRef(intern(path))] = file.mtime;
    }
    result.indexes.push_back(std::move(entry));
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

void reflect(JsonReader &vis, IndexInlayHint &v) {
  std::string t = vis.getString();
  char *s = const_cast<char *>(t.c_str());
  v.pos = Pos::fromString(t);
  s = strchr(s, '|');
  v.kind = static_cast<InlayHintKind>(strtol(s + 1, &s, 10));
  v.label = intern(s + 1);
}

void reflect(JsonWriter &vis, SymbolRef &v) {
  char buf[99];
  snprintf(buf, sizeof buf, "%s|%" PRIu64 "|%d|%d", v.range.toString().c_str(), v.usr, int(v.kind), int(v.role));
  std::string s(buf);
  reflect(vis, s);
}
void reflect(JsonWriter &vis, Use &v) {
  char buf[99];
  snprintf(buf, sizeof buf, "%s|%d|%d", v.range.toString().c_str(), int(v.role), v.file_id);
  std::string s(buf);
  reflect(vis, s);
}
void reflect(JsonWriter &vis, DeclRef &v) {
  char buf[99];
  snprintf(buf, sizeof buf, "%s|%s|%d|%d", v.range.toString().c_str(), v.extent.toString().c_str(), int(v.role),
           v.file_id);
  std::string s(buf);
  reflect(vis, s);
}
void reflect(JsonWriter &vis, IndexInlayHint &v) {
  std::string s = v.pos.toString() + "|" + std::to_string(int(v.kind)) + "|" + v.label;
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

void reflect(BinaryReader &vis, IndexInlayHint &v) {
  reflect(vis, v.pos);
  reflect(vis, v.kind);
  reflect(vis, v.label);
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
void reflect(BinaryWriter &vis, IndexInlayHint &v) {
  reflect(vis, v.pos);
  reflect(vis, v.kind);
  reflect(vis, v.label);
}
} // namespace ccls
