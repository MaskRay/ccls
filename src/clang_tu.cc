/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "clang_tu.hh"

#include "config.hh"
#include "platform.hh"

#include <clang/AST/Type.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/Path.h>

using namespace clang;

namespace ccls {
std::string PathFromFileEntry(const FileEntry &file) {
  StringRef Name = file.tryGetRealPathName();
  if (Name.empty())
    Name = file.getName();
  std::string ret = NormalizePath(Name);
  // Resolve /usr/include/c++/7.3.0 symlink.
  if (!llvm::any_of(g_config->workspaceFolders, [&](const std::string &root) {
        return StringRef(ret).startswith(root);
      })) {
    SmallString<256> dest;
    llvm::sys::fs::real_path(ret, dest);
    ret = llvm::sys::path::convert_to_slash(dest.str());
  }
  return ret;
}

static Pos Decomposed2LineAndCol(const SourceManager &SM,
                                 std::pair<FileID, unsigned> I) {
  int l = SM.getLineNumber(I.first, I.second) - 1,
      c = SM.getColumnNumber(I.first, I.second) - 1;
  return {(int16_t)std::min<int>(l, INT16_MAX),
          (int16_t)std::min<int>(c, INT16_MAX)};
}

Range FromCharSourceRange(const SourceManager &SM, const LangOptions &LangOpts,
                          CharSourceRange R,
                          llvm::sys::fs::UniqueID *UniqueID) {
  SourceLocation BLoc = R.getBegin(), ELoc = R.getEnd();
  std::pair<FileID, unsigned> BInfo = SM.getDecomposedLoc(BLoc),
                              EInfo = SM.getDecomposedLoc(ELoc);
  if (R.isTokenRange())
    EInfo.second += Lexer::MeasureTokenLength(ELoc, SM, LangOpts);
  if (UniqueID) {
    if (const FileEntry *F = SM.getFileEntryForID(BInfo.first))
      *UniqueID = F->getUniqueID();
    else
      *UniqueID = llvm::sys::fs::UniqueID(0, 0);
  }
  return {Decomposed2LineAndCol(SM, BInfo), Decomposed2LineAndCol(SM, EInfo)};
}

Range FromCharRange(const SourceManager &SM, const LangOptions &Lang,
                    SourceRange R, llvm::sys::fs::UniqueID *UniqueID) {
  return FromCharSourceRange(SM, Lang, CharSourceRange::getCharRange(R),
                             UniqueID);
}

Range FromTokenRange(const SourceManager &SM, const LangOptions &Lang,
                     SourceRange R, llvm::sys::fs::UniqueID *UniqueID) {
  return FromCharSourceRange(SM, Lang, CharSourceRange::getTokenRange(R),
                             UniqueID);
}

Range FromTokenRangeDefaulted(const SourceManager &SM, const LangOptions &Lang,
                              SourceRange R, const FileEntry *FE, Range range) {
  auto I = SM.getDecomposedLoc(SM.getExpansionLoc(R.getBegin()));
  if (SM.getFileEntryForID(I.first) == FE)
    range.start = Decomposed2LineAndCol(SM, I);
  SourceLocation L = SM.getExpansionLoc(R.getEnd());
  I = SM.getDecomposedLoc(L);
  if (SM.getFileEntryForID(I.first) == FE) {
    I.second += Lexer::MeasureTokenLength(L, SM, Lang);
    range.end = Decomposed2LineAndCol(SM, I);
  }
  return range;
}

std::unique_ptr<CompilerInvocation>
BuildCompilerInvocation(const std::string &main, std::vector<const char *> args,
                        IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS) {
  std::string save = "-resource-dir=" + g_config->clang.resourceDir;
  args.push_back(save.c_str());
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags(
      CompilerInstance::createDiagnostics(new DiagnosticOptions,
                                          new IgnoringDiagConsumer, true));
  std::unique_ptr<CompilerInvocation> CI =
      createInvocationFromCommandLine(args, Diags, VFS);
  if (CI) {
    CI->getDiagnosticOpts().IgnoreWarnings = true;
    CI->getFrontendOpts().DisableFree = false;
    CI->getLangOpts()->SpellChecking = false;
    auto &IS = CI->getFrontendOpts().Inputs;
    if (IS.size())
      IS[0] = FrontendInputFile(main, IS[0].getKind(), IS[0].isSystem());
  }
  return CI;
}

// clang::BuiltinType::getName without PrintingPolicy
const char *ClangBuiltinTypeName(int kind) {
  switch (BuiltinType::Kind(kind)) {
  case BuiltinType::Void:
    return "void";
  case BuiltinType::Bool:
    return "bool";
  case BuiltinType::Char_S:
    return "char";
  case BuiltinType::Char_U:
    return "char";
  case BuiltinType::SChar:
    return "signed char";
  case BuiltinType::Short:
    return "short";
  case BuiltinType::Int:
    return "int";
  case BuiltinType::Long:
    return "long";
  case BuiltinType::LongLong:
    return "long long";
  case BuiltinType::Int128:
    return "__int128";
  case BuiltinType::UChar:
    return "unsigned char";
  case BuiltinType::UShort:
    return "unsigned short";
  case BuiltinType::UInt:
    return "unsigned int";
  case BuiltinType::ULong:
    return "unsigned long";
  case BuiltinType::ULongLong:
    return "unsigned long long";
  case BuiltinType::UInt128:
    return "unsigned __int128";
  case BuiltinType::Half:
    return "__fp16";
  case BuiltinType::Float:
    return "float";
  case BuiltinType::Double:
    return "double";
  case BuiltinType::LongDouble:
    return "long double";
#if LLVM_VERSION_MAJOR >= 7
  case BuiltinType::ShortAccum:
    return "short _Accum";
  case BuiltinType::Accum:
    return "_Accum";
  case BuiltinType::LongAccum:
    return "long _Accum";
  case BuiltinType::UShortAccum:
    return "unsigned short _Accum";
  case BuiltinType::UAccum:
    return "unsigned _Accum";
  case BuiltinType::ULongAccum:
    return "unsigned long _Accum";
  case BuiltinType::BuiltinType::ShortFract:
    return "short _Fract";
  case BuiltinType::BuiltinType::Fract:
    return "_Fract";
  case BuiltinType::BuiltinType::LongFract:
    return "long _Fract";
  case BuiltinType::BuiltinType::UShortFract:
    return "unsigned short _Fract";
  case BuiltinType::BuiltinType::UFract:
    return "unsigned _Fract";
  case BuiltinType::BuiltinType::ULongFract:
    return "unsigned long _Fract";
  case BuiltinType::BuiltinType::SatShortAccum:
    return "_Sat short _Accum";
  case BuiltinType::BuiltinType::SatAccum:
    return "_Sat _Accum";
  case BuiltinType::BuiltinType::SatLongAccum:
    return "_Sat long _Accum";
  case BuiltinType::BuiltinType::SatUShortAccum:
    return "_Sat unsigned short _Accum";
  case BuiltinType::BuiltinType::SatUAccum:
    return "_Sat unsigned _Accum";
  case BuiltinType::BuiltinType::SatULongAccum:
    return "_Sat unsigned long _Accum";
  case BuiltinType::BuiltinType::SatShortFract:
    return "_Sat short _Fract";
  case BuiltinType::BuiltinType::SatFract:
    return "_Sat _Fract";
  case BuiltinType::BuiltinType::SatLongFract:
    return "_Sat long _Fract";
  case BuiltinType::BuiltinType::SatUShortFract:
    return "_Sat unsigned short _Fract";
  case BuiltinType::BuiltinType::SatUFract:
    return "_Sat unsigned _Fract";
  case BuiltinType::BuiltinType::SatULongFract:
    return "_Sat unsigned long _Fract";
#endif
  case BuiltinType::Float16:
    return "_Float16";
  case BuiltinType::Float128:
    return "__float128";
  case BuiltinType::WChar_S:
  case BuiltinType::WChar_U:
    return "wchar_t";
#if LLVM_VERSION_MAJOR >= 7
  case BuiltinType::Char8:
    return "char8_t";
#endif
  case BuiltinType::Char16:
    return "char16_t";
  case BuiltinType::Char32:
    return "char32_t";
  case BuiltinType::NullPtr:
    return "nullptr_t";
  case BuiltinType::Overload:
    return "<overloaded function type>";
  case BuiltinType::BoundMember:
    return "<bound member function type>";
  case BuiltinType::PseudoObject:
    return "<pseudo-object type>";
  case BuiltinType::Dependent:
    return "<dependent type>";
  case BuiltinType::UnknownAny:
    return "<unknown type>";
  case BuiltinType::ARCUnbridgedCast:
    return "<ARC unbridged cast type>";
  case BuiltinType::BuiltinFn:
    return "<builtin fn type>";
  case BuiltinType::ObjCId:
    return "id";
  case BuiltinType::ObjCClass:
    return "Class";
  case BuiltinType::ObjCSel:
    return "SEL";
  case BuiltinType::OCLSampler:
    return "sampler_t";
  case BuiltinType::OCLEvent:
    return "event_t";
  case BuiltinType::OCLClkEvent:
    return "clk_event_t";
  case BuiltinType::OCLQueue:
    return "queue_t";
  case BuiltinType::OCLReserveID:
    return "reserve_id_t";
  case BuiltinType::OMPArraySection:
    return "<OpenMP array section type>";
  default:
    return "";
  }
}
} // namespace ccls
