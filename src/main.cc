#include "log.hh"
#include "pipeline.hh"
#include "platform.h"
#include "serializer.h"
#include "serializers/json.h"
#include "test.h"
#include "working_files.h"
using namespace ccls;

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/Signals.h>
using namespace llvm;
using namespace llvm::cl;

#include <rapidjson/error/en.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>

std::string g_init_options;

namespace {
opt<bool> opt_help("h", desc("Alias for -help"));
opt<int> opt_verbose("v", desc("verbosity"), init(0));
opt<std::string> opt_test_index("test-index", ValueOptional, init("!"),
                                desc("run index tests"));

opt<std::string> opt_init("init", desc("extra initialization options"));
opt<std::string> opt_log_file("log-file", desc("log"), value_desc("filename"));
opt<std::string> opt_log_file_append("log-file-append", desc("log"),
                                     value_desc("filename"));

list<std::string> opt_extra(Positional, ZeroOrMore, desc("extra"));

void CloseLog() { fclose(ccls::log::file); }

} // namespace

int main(int argc, char **argv) {
  TraceMe();
  sys::PrintStackTraceOnErrorSignal(argv[0]);

  ParseCommandLineOptions(argc, argv,
                          "C/C++/Objective-C language server\n\n"
                          "See more on https://github.com/MaskRay/ccls/wiki");

  if (opt_help) {
    PrintHelpMessage();
    return 0;
  }

  pipeline::Init();
  const char *env = getenv("CCLS_CRASH_RECOVERY");
  if (!env || strcmp(env, "0") != 0)
    CrashRecoveryContext::Enable();

  bool language_server = true;

  if (opt_log_file.size() || opt_log_file_append.size()) {
    ccls::log::file = opt_log_file.size()
                          ? fopen(opt_log_file.c_str(), "wb")
                          : fopen(opt_log_file_append.c_str(), "ab");
    if (!ccls::log::file) {
      fprintf(
          stderr, "failed to open %s\n",
          (opt_log_file.size() ? opt_log_file : opt_log_file_append).c_str());
      return 2;
    }
    setbuf(ccls::log::file, NULL);
    atexit(CloseLog);
  }

  if (opt_test_index != "!") {
    language_server = false;
    if (!RunIndexTests(opt_test_index, sys::Process::StandardInIsUserInput()))
      return 1;
  }

  if (language_server) {
    if (!opt_init.empty()) {
      // We check syntax error here but override client-side
      // initializationOptions in messages/initialize.cc
      g_init_options = opt_init;
      rapidjson::Document reader;
      rapidjson::ParseResult ok = reader.Parse(g_init_options.c_str());
      if (!ok) {
        fprintf(stderr, "Failed to parse --init as JSON: %s (%zd)\n",
                rapidjson::GetParseError_En(ok.Code()), ok.Offset());
        return 1;
      }
      JsonReader json_reader{&reader};
      try {
        Config config;
        Reflect(json_reader, config);
      } catch (std::invalid_argument &e) {
        fprintf(stderr, "Failed to parse --init %s, expected %s\n",
                static_cast<JsonReader &>(json_reader).GetPath().c_str(),
                e.what());
        return 1;
      }
    }

    sys::ChangeStdinToBinary();
    sys::ChangeStdoutToBinary();
    // The thread that reads from stdin and dispatchs commands to the main
    // thread.
    pipeline::LaunchStdin();
    // The thread that writes responses from the main thread to stdout.
    pipeline::LaunchStdout();
    // Main thread which also spawns indexer threads upon the "initialize"
    // request.
    pipeline::MainLoop();
  }

  return 0;
}
