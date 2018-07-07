#include "indexer.h"

#include "log.hh"
#include "platform.h"
#include "serializer.h"
using ccls::Intern;

#include <clang/AST/AST.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Index/IndexDataConsumer.h>
#include <clang/Index/IndexingAction.h>
#include <clang/Index/USRGeneration.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/Support/Timer.h>
#include <llvm/Support/CrashRecoveryContext.h>
using namespace clang;
using llvm::Timer;

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <algorithm>
#include <map>
#include <chrono>
#include <unordered_set>

namespace {

struct IndexParam {
  std::map<llvm::sys::fs::UniqueID, std::string> SeenUniqueID;
  std::unordered_map<std::string, FileContents> file_contents;
  std::unordered_map<std::string, int64_t> file2write_time;
  llvm::DenseMap<const Decl*, Usr> Decl2usr;

  // Only use this when strictly needed (ie, primary translation unit is
  // needed). Most logic should get the IndexFile instance via
  // |file_consumer|.
  //
  // This can be null if we're not generating an index for the primary
  // translation unit.
  IndexFile* primary_file = nullptr;

  ASTUnit& Unit;
  ASTContext* Ctx;

  FileConsumer* file_consumer = nullptr;
  NamespaceHelper ns;

  IndexParam(ASTUnit& Unit, FileConsumer* file_consumer)
      : Unit(Unit), file_consumer(file_consumer) {}

  IndexFile *ConsumeFile(const FileEntry &File) {
    IndexFile *db =
      file_consumer->TryConsumeFile(File, &file_contents);

    // If this is the first time we have seen the file (ignoring if we are
    // generating an index for it):
    auto it = SeenUniqueID.try_emplace(File.getUniqueID());
    if (it.second) {
      std::string file_name = FileName(File);
      // Add to all files we have seen so we can generate proper dependency
      // graph.
      it.first->second = file_name;

      // Set modification time.
      std::optional<int64_t> write_time = LastWriteTime(file_name);
      LOG_IF_S(ERROR, !write_time)
        << "failed to fetch write time for " << file_name;
      if (write_time)
        file2write_time[file_name] = *write_time;
    }

    return db;
  }
};

Range FromSourceRange(const SourceManager &SM, const LangOptions &LangOpts,
                      SourceRange R, llvm::sys::fs::UniqueID *UniqueID,
                      bool token) {
  SourceLocation BLoc = R.getBegin(), ELoc = R.getEnd();
  std::pair<FileID, unsigned> BInfo = SM.getDecomposedLoc(BLoc);
  std::pair<FileID, unsigned> EInfo = SM.getDecomposedLoc(ELoc);
  if (token)
    EInfo.second += Lexer::MeasureTokenLength(ELoc, SM, LangOpts);
  unsigned l0 = SM.getLineNumber(BInfo.first, BInfo.second) - 1,
           c0 = SM.getColumnNumber(BInfo.first, BInfo.second) - 1,
           l1 = SM.getLineNumber(EInfo.first, EInfo.second) - 1,
           c1 = SM.getColumnNumber(EInfo.first, EInfo.second) - 1;
  if (l0 > INT16_MAX) l0 = 0;
  if (c0 > INT16_MAX) c0 = 0;
  if (l1 > INT16_MAX) l1 = 0;
  if (c1 > INT16_MAX) c1 = 0;
  if (UniqueID) {
    if (const FileEntry *F = SM.getFileEntryForID(BInfo.first))
      *UniqueID = F->getUniqueID();
    else
      *UniqueID = llvm::sys::fs::UniqueID(0, 0);
  }
  return {{int16_t(l0), int16_t(c0)}, {int16_t(l1), int16_t(c1)}};
}

Range FromCharRange(const SourceManager &SM, const LangOptions &LangOpts,
                    SourceRange R,
                    llvm::sys::fs::UniqueID *UniqueID = nullptr) {
  return FromSourceRange(SM, LangOpts, R, UniqueID, false);
}

Range FromTokenRange(const SourceManager &SM, const LangOptions &LangOpts,
                     SourceRange R,
                     llvm::sys::fs::UniqueID *UniqueID = nullptr) {
  return FromSourceRange(SM, LangOpts, R, UniqueID, true);
}

SymbolKind GetSymbolKind(const Decl* D) {
  switch (D->getKind()) {
  case Decl::TranslationUnit:
    return SymbolKind::File;
  case Decl::FunctionTemplate:
  case Decl::Function:
  case Decl::CXXMethod:
  case Decl::CXXConstructor:
  case Decl::CXXConversion:
  case Decl::CXXDestructor:
    return SymbolKind::Func;
  case Decl::Namespace:
  case Decl::NamespaceAlias:
  case Decl::ClassTemplate:
  case Decl::TypeAliasTemplate:
  case Decl::Enum:
  case Decl::Record:
  case Decl::CXXRecord:
  case Decl::ClassTemplateSpecialization:
  case Decl::TypeAlias:
  case Decl::Typedef:
  case Decl::UnresolvedUsingTypename:
    return SymbolKind::Type;
  case Decl::Field:
  case Decl::Var:
  case Decl::ParmVar:
  case Decl::ImplicitParam:
  case Decl::Decomposition:
  case Decl::EnumConstant:
    return SymbolKind::Var;
  default:
    return SymbolKind::Invalid;
  }
}

const Decl* GetTypeDecl(QualType T) {
  Decl *D = nullptr;
  const Type *TP;
  for(;;) {
    T = T.getUnqualifiedType();
    TP = T.getTypePtrOrNull();
    if (!TP)
      return D;
    switch (TP->getTypeClass()) {
    case Type::Pointer:
      T = cast<PointerType>(TP)->getPointeeType();
      continue;
    case Type::BlockPointer:
      T = cast<BlockPointerType>(TP)->getPointeeType();
      continue;
    case Type::LValueReference:
    case Type::RValueReference:
      T = cast<ReferenceType>(TP)->getPointeeType();
      continue;
    case Type::ObjCObjectPointer:
      T = cast<ObjCObjectPointerType>(TP)->getPointeeType();
      continue;
    case Type::MemberPointer:
      T = cast<MemberPointerType>(TP)->getPointeeType();
      continue;
    default:
      break;
    }
    break;
  }

try_again:
  switch (TP->getTypeClass()) {
  case Type::Typedef:
    D = cast<TypedefType>(TP)->getDecl();
    break;
  case Type::ObjCObject:
    D = cast<ObjCObjectType>(TP)->getInterface();
    break;
  case Type::ObjCInterface:
    D = cast<ObjCInterfaceType>(TP)->getDecl();
    break;
  case Type::Record:
  case Type::Enum:
    D = cast<TagType>(TP)->getDecl();
    break;
  case Type::TemplateSpecialization:
    if (const RecordType *Record = TP->getAs<RecordType>())
      D = Record->getDecl();
    else
      D = cast<TemplateSpecializationType>(TP)
              ->getTemplateName()
              .getAsTemplateDecl();
    break;

  case Type::Auto:
  case Type::DeducedTemplateSpecialization:
    TP = cast<DeducedType>(TP)->getDeducedType().getTypePtrOrNull();
    if (TP)
      goto try_again;
    break;

  case Type::InjectedClassName:
    D = cast<InjectedClassNameType>(TP)->getDecl();
    break;

  // FIXME: Template type parameters!

  case Type::Elaborated:
    TP = cast<ElaboratedType>(TP)->getNamedType().getTypePtrOrNull();
    goto try_again;

  default:
    break;
  }
  return D;
}

const Decl* GetSpecialized(const Decl* D) {
  if (!D)
    return D;
  Decl *Template = nullptr;
  if (const CXXRecordDecl *CXXRecord = dyn_cast<CXXRecordDecl>(D)) {
    if (const ClassTemplatePartialSpecializationDecl *PartialSpec
          = dyn_cast<ClassTemplatePartialSpecializationDecl>(CXXRecord))
      Template = PartialSpec->getSpecializedTemplate();
    else if (const ClassTemplateSpecializationDecl *ClassSpec
               = dyn_cast<ClassTemplateSpecializationDecl>(CXXRecord)) {
      llvm::PointerUnion<ClassTemplateDecl *,
                         ClassTemplatePartialSpecializationDecl *> Result
        = ClassSpec->getSpecializedTemplateOrPartial();
      if (Result.is<ClassTemplateDecl *>())
        Template = Result.get<ClassTemplateDecl *>();
      else
        Template = Result.get<ClassTemplatePartialSpecializationDecl *>();

    } else
      Template = CXXRecord->getInstantiatedFromMemberClass();
  } else if (const FunctionDecl *Function = dyn_cast<FunctionDecl>(D)) {
    Template = Function->getPrimaryTemplate();
    if (!Template)
      Template = Function->getInstantiatedFromMemberFunction();
  } else if (const VarDecl *Var = dyn_cast<VarDecl>(D)) {
    if (Var->isStaticDataMember())
      Template = Var->getInstantiatedFromStaticDataMember();
  } else if (const RedeclarableTemplateDecl *Tmpl
                                        = dyn_cast<RedeclarableTemplateDecl>(D))
    Template = Tmpl->getInstantiatedFromMemberTemplate();
  else
    return nullptr;
  return Template;
}

class IndexDataConsumer : public index::IndexDataConsumer {
public:
  ASTContext *Ctx;
  IndexParam& param;

  std::string GetComment(const Decl* D) {
    SourceManager &SM = Ctx->getSourceManager();
    const RawComment *RC = Ctx->getRawCommentForAnyRedecl(D);
    if (!RC) return "";
    StringRef Raw = RC->getRawText(Ctx->getSourceManager());
    SourceRange R = RC->getSourceRange();
    std::pair<FileID, unsigned> BInfo = SM.getDecomposedLoc(R.getBegin());
    unsigned start_column = SM.getLineNumber(BInfo.first, BInfo.second);
    std::string ret;
    int pad = -1;
    for (const char *p = Raw.data(), *E = Raw.end(); p < E;) {
      // The first line starts with a comment marker, but the rest needs
      // un-indenting.
      unsigned skip = start_column - 1;
      for (; skip > 0 && p < E && (*p == ' ' || *p == '\t'); p++)
        skip--;
      const char *q = p;
      while (q < E && *q != '\n')
        q++;
      if (q < E)
        q++;
      // A minimalist approach to skip Doxygen comment markers.
      // See https://www.stack.nl/~dimitri/doxygen/manual/docblocks.html
      if (pad < 0) {
        // First line, detect the length of comment marker and put into |pad|
        const char *begin = p;
        while (p < E && (*p == '/' || *p == '*'))
          p++;
        if (p < E && (*p == '<' || *p == '!'))
          p++;
        if (p < E && *p == ' ')
          p++;
        pad = int(p - begin);
      } else {
        // Other lines, skip |pad| bytes
        int prefix = pad;
        while (prefix > 0 && p < E &&
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

  Usr GetUsr(const Decl* D) const {
    D = D->getCanonicalDecl();
    auto R = param.Decl2usr.try_emplace(D);
    if (R.second) {
      SmallString<256> USR;
      index::generateUSRForDecl(D, USR);
      R.first->second = HashUsr(USR);
    }
    return R.first->second;
  }

  Use GetUse(IndexFile *db, Range range, const DeclContext *DC,
             Role role) const {
    if (!DC)
      return Use{{range, 0, SymbolKind::File, role}};
    const Decl *D = cast<Decl>(DC);
    switch (GetSymbolKind(D)) {
    case SymbolKind::Func:
      return Use{{range, db->ToFunc(GetUsr(D)).usr, SymbolKind::Func, role}};
    case SymbolKind::Type:
      return Use{{range, db->ToType(GetUsr(D)).usr, SymbolKind::Type, role}};
    case SymbolKind::Var:
      return Use{{range, db->ToVar(GetUsr(D)).usr, SymbolKind::Var, role}};
    default:
      return Use{{range, 0, SymbolKind::File, role}};
    }
  }

  PrintingPolicy GetDefaultPolicy() const {
    PrintingPolicy PP(Ctx->getLangOpts());
    PP.AnonymousTagLocations = false;
    PP.TerseOutput = true;
    PP.PolishForDeclaration = true;
    PP.ConstantsAsWritten = true;
    PP.SuppressTagKeyword = true;
    PP.SuppressInitializers = true;
    PP.FullyQualifiedName = false;
    return PP;
  }

  static void SimplifyAnonymous(std::string& name) {
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
  void SetName(const Decl *D, std::string_view short_name,
               std::string_view qualified, Def &def, bool hover = false) {
    SmallString<256> Str;
    llvm::raw_svector_ostream OS(Str);
    PrintingPolicy PP = GetDefaultPolicy();
    if (hover)
      PP.SuppressInitializers = false;
    D->print(OS, PP);

    std::string name = OS.str();
    SimplifyAnonymous(name);
    auto i = name.find(short_name);
    if (i == std::string::npos) {
      // e.g. operator type-parameter-1
      i = 0;
      if (!hover)
        def.short_name_offset = 0;
    } else if (short_name.size()) {
      name.replace(i, short_name.size(), qualified);
      if (!hover)
        def.short_name_offset = i + qualified.size() - short_name.size();
    } else {
      if (!hover)
        def.short_name_offset = i;
    }
    if (hover) {
      if (name != def.detailed_name)
        def.hover = Intern(name);
    } else {
      def.short_name_size = short_name.size();
      for (int paren = 0; i; i--) {
        // Skip parentheses in "(anon struct)::name"
        if (name[i - 1] == ')')
          paren++;
        else if (name[i - 1] == '(')
          paren--;
        else if (!(paren > 0 || isalnum(name[i - 1]) || name[i - 1] == '_' ||
            name[i - 1] == ':'))
          break;
      }
      def.qual_name_offset = i;
      def.detailed_name = Intern(name);
    }
  }

  void AddMacroUse(SourceManager &SM, std::vector<Use> &uses,
                   SourceLocation Spell) const {
    const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(Spell));
    if (FE) {
      IndexFile *db = param.ConsumeFile(*FE);
      Range spell =
          FromTokenRange(SM, Ctx->getLangOpts(), SourceRange(Spell, Spell));
      uses.push_back(GetUse(db, spell, nullptr, Role::Dynamic));
    }
  }

public:
  IndexDataConsumer(IndexParam& param) : param(param) {}
  void initialize(ASTContext &Ctx) override {
    this->Ctx = param.Ctx = &Ctx;
  }
  bool handleDeclOccurence(const Decl *D, index::SymbolRoleSet Roles,
                           ArrayRef<index::SymbolRelation> Relations,
#if LLVM_VERSION_MAJOR >= 7
                           SourceLocation Loc,
#else
                           FileID LocFID, unsigned LocOffset,
#endif
                           ASTNodeInfo ASTNode) override {
    SourceManager &SM = Ctx->getSourceManager();
    const LangOptions &Lang = Ctx->getLangOpts();
#if LLVM_VERSION_MAJOR < 7
    SourceLocation Loc;
    {
      const SrcMgr::SLocEntry &Entry = SM.getSLocEntry(LocFID);
      unsigned off = Entry.getOffset() + LocOffset;
      if (!Entry.isFile())
        off |= 1u << 31;
      Loc = SourceLocation::getFromRawEncoding(off);
    }
#endif
    SourceLocation Spell = SM.getSpellingLoc(Loc);
    Loc = SM.getFileLoc(Loc);
    Range loc = FromTokenRange(SM, Ctx->getLangOpts(), SourceRange(Loc, Loc));
    const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(Loc));
    if (!FE) {
      // TODO
#if LLVM_VERSION_MAJOR < 7
      auto P = SM.getExpansionRange(Loc);
      loc = FromCharRange(SM, Ctx->getLangOpts(), SourceRange(P.first, P.second));
      FE = SM.getFileEntryForID(SM.getFileID(P.first));
#else
      auto R = SM.getExpansionRange(Loc);
      loc = FromTokenRange(SM, Ctx->getLangOpts(), R.getAsRange());
      FE = SM.getFileEntryForID(SM.getFileID(R.getBegin()));
#endif
      if (!FE)
        return true;
    }
    IndexFile *db = param.ConsumeFile(*FE);
    if (!db)
      return true;

    const DeclContext *SemDC = D->getDeclContext();
    const DeclContext *LexDC = D->getLexicalDeclContext();
    (void)SemDC;
    (void)LexDC;
    Range extent = FromTokenRange(SM, Lang, D->getSourceRange());
    Role role = static_cast<Role>(Roles);

    bool is_decl = Roles & uint32_t(index::SymbolRole::Declaration);
    bool is_def = Roles & uint32_t(index::SymbolRole::Definition);
    std::string short_name, qualified;

    if (auto* ND = dyn_cast<NamedDecl>(D)) {
      short_name = ND->getNameAsString();
      qualified = ND->getQualifiedNameAsString();
      SimplifyAnonymous(qualified);
    }

    IndexFunc *func = nullptr;
    IndexType *type = nullptr;
    IndexVar *var = nullptr;
    SymbolKind kind = GetSymbolKind(D);
    Usr usr = GetUsr(D);

    auto do_def_decl = [&](auto *entity) {
      if (!entity->def.detailed_name[0]) {
        SetName(D, short_name, qualified, entity->def);
        if (g_config->index.comments)
          entity->def.comments = Intern(GetComment(D));
      }
      if (is_def) {
        entity->def.spell = GetUse(db, loc, LexDC, role);
        // extent may come from a declaration.
        entity->def.extent = GetUse(db, extent, LexDC, Role::None);
      } else if (is_decl) {
        entity->declarations.push_back(GetUse(db, loc, LexDC, role));
      } else {
        entity->uses.push_back(GetUse(db, loc, LexDC, role));
      }
      if (Spell != Loc)
        AddMacroUse(SM, entity->uses, Spell);
    };
    switch (kind) {
    case SymbolKind::Invalid:
      LOG_S(INFO) << "Unhandled " << int(D->getKind());
      return true;
    case SymbolKind::File:
      return true;
    case SymbolKind::Func:
      func = &db->ToFunc(usr);
      do_def_decl(func);
      if (is_def || is_decl) {
        const Decl* DC = cast<Decl>(SemDC);
        if (GetSymbolKind(DC) == SymbolKind::Type)
          db->ToType(GetUsr(DC)).def.funcs.push_back(usr);
      }
      break;
    case SymbolKind::Type:
      type = &db->ToType(usr);
      do_def_decl(type);
      if (is_def || is_decl) {
        const Decl* DC = cast<Decl>(SemDC);
        if (GetSymbolKind(DC) == SymbolKind::Type)
          db->ToType(GetUsr(DC)).def.types.push_back(usr);
      }
      break;
    case SymbolKind::Var:
      var = &db->ToVar(usr);
      do_def_decl(var);
      if (is_def || is_decl) {
        const Decl* DC = cast<Decl>(SemDC);
        if (GetSymbolKind(DC) == SymbolKind::Type)
          db->ToFunc(GetUsr(DC)).def.vars.push_back(usr);
        else if (auto *ND = dyn_cast<NamespaceDecl>(SemDC))
          db->ToType(GetUsr(ND)).def.vars.emplace_back(usr, -1);
        QualType T;
        if (auto *VD = dyn_cast<VarDecl>(D)) {
          var->def.storage = VD->getStorageClass();
          T = VD->getType();
          // O(n^2)
          for (auto I : VD->redecls())
            if (I->hasInit()) {
              // TODO missing "static" in definition
              SetName(I, short_name, qualified, var->def, true);
              break;
            }
        } else if (auto *FD = dyn_cast<FieldDecl>(D)) {
          T = FD->getType();
          if (FD->hasInClassInitializer())
            SetName(D, short_name, qualified, var->def, true);
        }
        if (!T.isNull()) {
          if (auto *BT = T->getAs<BuiltinType>()) {
            Usr usr1 = static_cast<Usr>(BT->getKind());
            var->def.type = usr1;
            db->ToType(usr1).instances.push_back(usr);
          } else {
            for (const Decl *D1 = GetTypeDecl(T); D1; D1 = GetSpecialized(D1)) {
              Usr usr1 = GetUsr(D1);
              auto it = db->usr2type.find(usr1);
              if (it != db->usr2type.end()) {
                var->def.type = usr1;
                it->second.instances.push_back(usr);
                break;
              }
            }
          }
        }
      }
      break;
    }

    switch (D->getKind()) {
    case Decl::Namespace:
      type->def.kind = lsSymbolKind::Namespace;
      break;
    case Decl::NamespaceAlias: {
      type->def.kind = lsSymbolKind::TypeAlias;
      auto* NAD = cast<NamespaceAliasDecl>(D);
      if (const NamespaceDecl* ND = NAD->getNamespace()) {
        Usr usr1 = GetUsr(ND);
        if (db->usr2type.count(usr1))
          type->def.alias_of = usr1;
      }
      break;
    }
    case Decl::Enum:
      type->def.kind = lsSymbolKind::Enum;
      break;
    case Decl::CXXRecord: {
      auto *RD = cast<CXXRecordDecl>(D);
      if (is_def && RD->hasDefinition()) {
        for (const CXXBaseSpecifier &Base : RD->bases()) {
          QualType T = Base.getType();
          const NamedDecl *BaseD = nullptr;
          if (auto *TDT = T->getAs<TypedefType>()) {
            BaseD = TDT->getDecl();
          } else if (auto *TST = T->getAs<TemplateSpecializationType>()) {
            BaseD = TST->getTemplateName().getAsTemplateDecl();
          } else if (auto *RT = T->getAs<RecordType>()) {
            BaseD = RT->getDecl();
          }
          if (BaseD) {
            Usr usr1 = GetUsr(BaseD);
            auto it = db->usr2type.find(usr1);
            if (it != db->usr2type.end()) {
              type->def.bases.push_back(usr1);
              it->second.derived.push_back(usr);
            }
          }
        }
      }
    }
      [[fallthrough]];
    case Decl::Record: {
      auto *RD = cast<RecordDecl>(D);
      // spec has no Union, use Class
      type->def.kind = RD->getTagKind() == TTK_Struct ? lsSymbolKind::Struct
                                                      : lsSymbolKind::Class;
      if (is_def) {
        bool can_get_offset =
            RD->isCompleteDefinition() && !RD->isDependentType();
        for (FieldDecl *FD : RD->fields())
          type->def.vars.emplace_back(
              GetUsr(FD), can_get_offset ? Ctx->getFieldOffset(FD) : -1);
      }
      break;
    }
    case Decl::ClassTemplate:
      type->def.kind = lsSymbolKind::Class;
      break;
    case Decl::FunctionTemplate:
      type->def.kind = lsSymbolKind::Function;
      break;
    case Decl::TypeAliasTemplate:
      type->def.kind = lsSymbolKind::TypeAlias;
      break;
    case Decl::ClassTemplateSpecialization:
      type->def.kind = lsSymbolKind::Class;
      if (is_def || is_decl) {
        if (auto *RD = dyn_cast<CXXRecordDecl>(D)) {
          Decl *D1 = nullptr;
          if (auto *SD = dyn_cast<ClassTemplatePartialSpecializationDecl>(RD))
            D1 = SD->getSpecializedTemplate();
          else if (auto *SD = dyn_cast<ClassTemplateSpecializationDecl>(RD)) {
            llvm::PointerUnion<ClassTemplateDecl *,
                               ClassTemplatePartialSpecializationDecl *>
                Result = SD->getSpecializedTemplateOrPartial();
            if (Result.is<ClassTemplateDecl *>())
              D1 = Result.get<ClassTemplateDecl *>();
            else
              D1 = Result.get<ClassTemplatePartialSpecializationDecl *>();

          } else
            D1 = RD->getInstantiatedFromMemberClass();
          if (D1) {
            Usr usr1 = GetUsr(D1);
            auto it = db->usr2type.find(usr1);
            if (it != db->usr2type.end()) {
              type->def.bases.push_back(usr1);
              it->second.derived.push_back(usr);
            }
          }
        }
      }
      break;
    case Decl::TypeAlias:
    case Decl::Typedef:
    case Decl::UnresolvedUsingTypename:
      type->def.kind = lsSymbolKind::TypeAlias;
      if (auto *TD = dyn_cast<TypedefNameDecl>(D)) {
        QualType T = TD->getUnderlyingType();
        if (const Decl* D1 = GetTypeDecl(T)) {
          Usr usr1 = GetUsr(D1);
          if (db->usr2type.count(usr1))
            type->def.alias_of = usr1;
        }
      }
      break;
    case Decl::Function:
      func->def.kind = lsSymbolKind::Function;
      break;
    case Decl::CXXMethod:
      func->def.kind = lsSymbolKind::Method;
      if (is_def || is_decl) {
        if (auto *ND = dyn_cast<NamedDecl>(D)) {
          SmallVector<const NamedDecl *, 8> OverDecls;
          Ctx->getOverriddenMethods(ND, OverDecls);
          for (const auto* ND1 : OverDecls) {
            Usr usr1 = GetUsr(ND1);
            auto it = db->usr2func.find(usr1);
            if (it != db->usr2func.end()) {
              func->def.bases.push_back(usr1);
              it->second.derived.push_back(usr);
            }
          }
        }
      }
      break;
    case Decl::CXXConstructor:
    case Decl::CXXConversion:
      func->def.kind = lsSymbolKind::Constructor;
      break;
    case Decl::CXXDestructor:
      func->def.kind = lsSymbolKind::Method;
      break;
    case Decl::Field:
      var->def.kind = lsSymbolKind::Field;
      break;
    case Decl::Var:
    case Decl::Decomposition:
      var->def.kind = lsSymbolKind::Variable;
      break;
    case Decl::ImplicitParam:
    case Decl::ParmVar:
      // ccls extension
      var->def.kind = lsSymbolKind::Parameter;
      break;
    case Decl::EnumConstant:
      var->def.kind = lsSymbolKind::EnumMember;
      if (is_def) {
        auto *ECD = cast<EnumConstantDecl>(D);
        const auto &Val = ECD->getInitVal();
        std::string init =
            " = " + (Val.isSigned() ? std::to_string(Val.getSExtValue())
                                    : std::to_string(Val.getZExtValue()));
        var->def.hover = Intern(var->def.detailed_name + init);
      }
      break;
    default:
      LOG_S(INFO) << "Unhandled " << int(D->getKind());
      break;
    }
    return true;
  }
};

class IndexPPCallbacks : public PPCallbacks {
  SourceManager& SM;
  IndexParam& param;

  std::pair<StringRef, Usr> GetMacro(const Token& Tok) const {
    StringRef Name = Tok.getIdentifierInfo()->getName();
    SmallString<256> USR("@macro@");
    USR += Name;
    return {Name, HashUsr(USR)};
  }

public:
  IndexPPCallbacks(SourceManager& SM, IndexParam& param) : SM(SM), param(param) {}
  void MacroDefined(const Token &Tok, const MacroDirective *MD) override {
    llvm::sys::fs::UniqueID UniqueID;
    SourceLocation L = MD->getLocation();
    auto range =
        FromTokenRange(SM, param.Ctx->getLangOpts(), {L, L}, &UniqueID);
    const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(L));
    if (!FE)
      return;
    if (IndexFile *db = param.ConsumeFile(*FE)) {
      auto[Name, usr] = GetMacro(Tok);
      IndexVar &var = db->ToVar(usr);
      if (!var.def.detailed_name[0]) {
        var.def.detailed_name = Intern(Name);
        var.def.short_name_size = Name.size();
        // TODO defin
        var.def.hover = Intern(Twine("#define ", Name).str());
        var.def.kind = lsSymbolKind::Macro;
        if (var.def.spell)
          var.declarations.push_back(*var.def.spell);
        var.def.spell = Use{{range, 0, SymbolKind::File, Role::Definition}};
        var.def.extent = var.def.spell;
      }
    }
  }
  void MacroExpands(const Token &Tok, const MacroDefinition &MD,
                    SourceRange R, const MacroArgs *Args) override {
    llvm::sys::fs::UniqueID UniqueID;
    auto range = FromTokenRange(SM, param.Ctx->getLangOpts(), R, &UniqueID);
    const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(R.getBegin()));
    if (!FE)
      return;
    if (IndexFile *db = param.ConsumeFile(*FE)) {
      auto[Name, usr] = GetMacro(Tok);
      IndexVar &var = db->ToVar(usr);
      var.uses.push_back({{range, 0, SymbolKind::File, Role::Reference}});
    }
  }
  void MacroUndefined(const Token &Tok, const MacroDefinition &MD,
                      const MacroDirective *UD) override {
    SourceLocation L = UD->getLocation();
    MacroExpands(Tok, MD, {L, L}, nullptr);
  }
  void SourceRangeSkipped(SourceRange Range, SourceLocation EndifLoc) override {
    llvm::sys::fs::UniqueID UniqueID;
    auto range = FromCharRange(SM, param.Ctx->getLangOpts(), Range, &UniqueID);
    const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(Range.getBegin()));
    if (IndexFile *db = param.ConsumeFile(*FE))
      db->skipped_ranges.push_back(range);
  }
};

class IndexFrontendAction : public ASTFrontendAction {
  IndexParam& param;
public:
  IndexFrontendAction(IndexParam& param) : param(param) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override {
    Preprocessor &PP = CI.getPreprocessor();
    PP.addPPCallbacks(std::make_unique<IndexPPCallbacks>(PP.getSourceManager(), param));
    return std::make_unique<ASTConsumer>();
  }
};
}

const int IndexFile::kMajorVersion = 16;
const int IndexFile::kMinorVersion = 1;

IndexFile::IndexFile(llvm::sys::fs::UniqueID UniqueID, const std::string &path,
                     const std::string &contents)
    : UniqueID(UniqueID), path(path), file_contents(contents) {}

IndexFunc& IndexFile::ToFunc(Usr usr) {
  auto ret = usr2func.try_emplace(usr);
  if (ret.second)
    ret.first->second.usr = usr;
  return ret.first->second;
}

IndexType& IndexFile::ToType(Usr usr) {
  auto ret = usr2type.try_emplace(usr);
  if (ret.second)
    ret.first->second.usr = usr;
  return ret.first->second;
}

IndexVar& IndexFile::ToVar(Usr usr) {
  auto ret = usr2var.try_emplace(usr);
  if (ret.second)
    ret.first->second.usr = usr;
  return ret.first->second;
}

std::string IndexFile::ToString() {
  return ccls::Serialize(SerializeFormat::Json, *this);
}

void Uniquify(std::vector<Usr>& usrs) {
  std::unordered_set<Usr> seen;
  size_t n = 0;
  for (size_t i = 0; i < usrs.size(); i++)
    if (seen.insert(usrs[i]).second)
      usrs[n++] = usrs[i];
  usrs.resize(n);
}

void Uniquify(std::vector<Use>& uses) {
  std::unordered_set<Range> seen;
  size_t n = 0;
  for (size_t i = 0; i < uses.size(); i++) {
    if (seen.insert(uses[i].range).second)
      uses[n++] = uses[i];
  }
  uses.resize(n);
}

std::vector<std::unique_ptr<IndexFile>> ClangIndexer::Index(
    VFS* vfs,
    std::string file,
    const std::vector<std::string>& args,
    const std::vector<FileContents>& file_contents) {
  if (!g_config->index.enabled)
    return {};

  file = NormalizePath(file);

  std::vector<const char *> Args;
  for (auto& arg: args)
    Args.push_back(arg.c_str());
  Args.push_back("-fno-spell-checking");
  auto PCHCO = std::make_shared<PCHContainerOperations>();
  IntrusiveRefCntPtr<DiagnosticsEngine>
    Diags(CompilerInstance::createDiagnostics(new DiagnosticOptions));
  std::shared_ptr<CompilerInvocation> CI =
      createInvocationFromCommandLine(Args, Diags);
  if (!CI)
    return {};
  CI->getLangOpts()->CommentOpts.ParseAllComments = true;
  CI->getLangOpts()->RetainCommentsFromSystemHeaders = true;

  std::vector<std::unique_ptr<llvm::MemoryBuffer>> BufOwner;
  for (auto &c : file_contents) {
    std::unique_ptr<llvm::MemoryBuffer> MB =
        llvm::MemoryBuffer::getMemBufferCopy(c.content, c.path);
    CI->getPreprocessorOpts().addRemappedFile(c.path, MB.get());
    BufOwner.push_back(std::move(MB));
  }

  auto Unit = ASTUnit::create(CI, Diags, true, true);
  if (!Unit)
    return {};

  FileConsumer file_consumer(vfs, file);
  IndexParam param(*Unit, &file_consumer);
  auto DataConsumer = std::make_shared<IndexDataConsumer>(param);

  index::IndexingOptions IndexOpts;
  memset(&IndexOpts, 1, sizeof IndexOpts);
  IndexOpts.SystemSymbolFilter =
      index::IndexingOptions::SystemSymbolFilterKind::All;
  IndexOpts.IndexFunctionLocals = true;

  std::unique_ptr<FrontendAction> IndexAction = createIndexingAction(
      DataConsumer, IndexOpts, std::make_unique<IndexFrontendAction>(param));
  llvm::CrashRecoveryContextCleanupRegistrar<FrontendAction> IndexActionCleanup(
      IndexAction.get());

  DiagnosticErrorTrap DiagTrap(*Diags);
  bool Success = ASTUnit::LoadFromCompilerInvocationAction(
      std::move(CI), PCHCO, Diags, IndexAction.get(), Unit.get(),
      /*Persistent=*/true, "/home/maskray/Dev/llvm/release/lib/clang/7.0.0",
      /*OnlyLocalDecls=*/true,
      /*CaptureDiagnostics=*/true, 0, false, false, true);
  if (!Unit) {
    LOG_S(ERROR) << "failed to index " << file;
    return {};
  }
  if (!Success)
    return {};

  // ClangCursor(clang_getTranslationUnitCursor(tu->cx_tu))
  //     .VisitChildren(&VisitMacroDefinitionAndExpansions, &param);
  const SourceManager& SM = Unit->getSourceManager();
  const FileEntry* FE = SM.getFileEntryForID(SM.getMainFileID());
  param.primary_file = param.ConsumeFile(*FE);
  std::unordered_map<std::string, int> inc_to_line;
  // TODO
  if (param.primary_file)
    for (auto& inc : param.primary_file->includes)
      inc_to_line[inc.resolved_path] = inc.line;

  auto result = param.file_consumer->TakeLocalState();
  for (std::unique_ptr<IndexFile>& entry : result) {
    entry->import_file = file;
    entry->args = args;
    for (auto& it : entry->usr2func) {
      // e.g. declaration + out-of-line definition
      Uniquify(it.second.derived);
      Uniquify(it.second.uses);
    }
    for (auto& it : entry->usr2type) {
      Uniquify(it.second.derived);
      Uniquify(it.second.uses);
      // e.g. declaration + out-of-line definition
      Uniquify(it.second.def.funcs);
    }
    for (auto& it : entry->usr2var)
      Uniquify(it.second.uses);

    if (param.primary_file) {
      // If there are errors, show at least one at the include position.
      auto it = inc_to_line.find(entry->path);
      if (it != inc_to_line.end()) {
        int line = it->second;
        for (auto ls_diagnostic : entry->diagnostics_) {
          if (ls_diagnostic.severity != lsDiagnosticSeverity::Error)
            continue;
          ls_diagnostic.range =
              lsRange{lsPosition{line, 10}, lsPosition{line, 10}};
          param.primary_file->diagnostics_.push_back(ls_diagnostic);
          break;
        }
      }
    }

    // Update file contents and modification time.
    entry->last_write_time = param.file2write_time[entry->path];

    // Update dependencies for the file. Do not include the file in its own
    // dependency set.
    for (auto & [ _, path ] : param.SeenUniqueID)
      if (path != entry->path && path != entry->import_file)
        entry->dependencies[path] = param.file2write_time[path];
  }

  return result;
}

void IndexInit() {
  // InitLLVM
  CXIndex CXIdx = clang_createIndex(0, 0);
  clang_disposeIndex(CXIdx);
}

// |SymbolRef| is serialized this way.
// |Use| also uses this though it has an extra field |file|,
// which is not used by Index* so it does not need to be serialized.
void Reflect(Reader& visitor, Reference& value) {
  if (visitor.Format() == SerializeFormat::Json) {
    std::string t = visitor.GetString();
    char* s = const_cast<char*>(t.c_str());
    value.range = Range::FromString(s);
    s = strchr(s, '|');
    value.usr = strtoull(s + 1, &s, 10);
    value.kind = static_cast<SymbolKind>(strtol(s + 1, &s, 10));
    value.role = static_cast<Role>(strtol(s + 1, &s, 10));
  } else {
    Reflect(visitor, value.range);
    Reflect(visitor, value.usr);
    Reflect(visitor, value.kind);
    Reflect(visitor, value.role);
  }
}
void Reflect(Writer& visitor, Reference& value) {
  if (visitor.Format() == SerializeFormat::Json) {
    char buf[99];
    snprintf(buf, sizeof buf, "%s|%" PRIu64 "|%d|%d",
             value.range.ToString().c_str(), value.usr, int(value.kind),
             int(value.role));
    std::string s(buf);
    Reflect(visitor, s);
  } else {
    Reflect(visitor, value.range);
    Reflect(visitor, value.usr);
    Reflect(visitor, value.kind);
    Reflect(visitor, value.role);
  }
}
