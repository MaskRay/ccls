#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Tool.h>
#include <clang/Driver/Types.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm-c/Target.h>

using namespace clang;

struct StoreDiags : DiagnosticConsumer {
  const LangOptions *langOpts;
  std::string message;

  void BeginSourceFile(const LangOptions &opts, const Preprocessor *) override { langOpts = &opts; }
  void HandleDiagnostic(DiagnosticsEngine::Level level, const clang::Diagnostic &info) override {
    DiagnosticConsumer::HandleDiagnostic(level, info);
    const char *prefix;
    switch (level) {
    default:
      return;
    case DiagnosticsEngine::Warning:
      prefix = "warning";
      break;
    case DiagnosticsEngine::Error:
      prefix = "error";
      break;
    case DiagnosticsEngine::Fatal:
      prefix = "fatal error";
      break;
    }

    llvm::SmallString<64> msg;
    info.FormatDiagnostic(msg);
    auto &sm = info.getSourceManager();
    std::pair<FileID, unsigned> i = sm.getDecomposedLoc(sm.getFileLoc(info.getLocation()));
    int l = (int)sm.getLineNumber(i.first, i.second), c = (int)sm.getColumnNumber(i.first, i.second);
    llvm::raw_string_ostream(message) << l << ':' << c << ": " << prefix << ": " << msg << '\n';
  }
};

std::pair<std::string, std::string> compile(llvm::StringRef source) {
  int fd_in = memfd_create("a.cc", 0), fd_out = memfd_create("a.o", 0);
  llvm::raw_fd_stream osIn(fd_in, true), osOut(fd_out, true);
  if (fd_in < 0 || fd_out < 0)
    return {"", "failed to open file"};
  osIn << source;
  osIn.seek(0);

  auto fs = llvm::vfs::getRealFileSystem();
  StoreDiags dc;
  char file_in[64], file_out[64];
  snprintf(file_in, sizeof(file_in), "/proc/self/fd/%d", fd_in);
  snprintf(file_out, sizeof(file_out), "/proc/self/fd/%d", fd_out);
  std::vector<const char *> args{
      "clang", "-c", "-Wall", "-Wextra", "-xc++", file_in, "-o", file_out, "-fno-ident"};
  llvm::IntrusiveRefCntPtr<DiagnosticsEngine> diags(CompilerInstance::createDiagnostics(
#if LLVM_VERSION_MAJOR >= 20
      *fs,
#endif
      new DiagnosticOptions, &dc, false));
  driver::Driver d(args[0], "x86_64", *diags, "cc", fs);
  d.setCheckInputsExist(false);
  std::unique_ptr<driver::Compilation> comp(d.BuildCompilation(args));
  const auto &jobs = comp->getJobs();
  assert(jobs.size() == 1 && isa<driver::Command>(*jobs.begin()));
  const llvm::opt::ArgStringList &cc_args = cast<driver::Command>(*jobs.begin()).getArguments();
  auto ci = std::make_unique<CompilerInvocation>();
  CompilerInvocation::CreateFromArgs(*ci, cc_args, *diags);
  ci->getFrontendOpts().DisableFree = false;

  auto clang = std::make_unique<CompilerInstance>(std::make_shared<PCHContainerOperations>());
  clang->setInvocation(std::move(ci));
  clang->createDiagnostics(
#if LLVM_VERSION_MAJOR >= 20
      *fs,
#endif
      &dc, false);
  diags.reset();
  clang->setTarget(TargetInfo::CreateTargetInfo(clang->getDiagnostics(), clang->getInvocation().TargetOpts));
  if (!clang->hasTarget())
    return {"", "unsupported target"};
  clang->createFileManager(fs);
  clang->setSourceManager(new SourceManager(clang->getDiagnostics(), clang->getFileManager(), true));

  LLVMInitializeX86AsmPrinter();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86TargetMC();

  auto action = std::make_unique<EmitObjAction>();
  action->BeginSourceFile(*clang, clang->getFrontendOpts().Inputs[0]);
  llvm::Error e = action->Execute();
  if (e)
    return {"", llvm::toString(std::move(e))};
  action->EndSourceFile();

  auto size = (long)lseek(fd_out, 0, SEEK_END);
  std::string ret(size, '\0');
  osOut.seek(0);
  osOut.read(ret.data(), size);
  return {std::move(ret), std::move(dc.message)};
}
