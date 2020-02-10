// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "log.hh"
#include "pipeline.hh"
#include "platform.hh"
#include "serializer.hh"
#include "test.hh"
#include "working_files.hh"

#include <clang/Basic/Version.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/Signals.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>

using namespace ccls;
using namespace llvm;
using namespace llvm::cl;

namespace ccls {
std::vector<std::string> g_init_options;
}

namespace {
OptionCategory C("ccls options");

opt<bool> opt_help("h", desc("Alias for -help"), cat(C));
opt<int> opt_verbose("v", desc("verbosity, from -3 (fatal) to 2 (verbose)"),
                     init(0), cat(C));
opt<std::string> opt_test_index("test-index", ValueOptional, init("!"),
                                desc("run index tests"), cat(C));

opt<std::string> opt_index("index",
                           desc("standalone mode: index a project and exit"),
                           value_desc("root"), cat(C));
list<std::string> opt_init("init", desc("extra initialization options in JSON"),
                           cat(C));
opt<std::string> opt_log_file("log-file", desc("stderr or log file"),
                              value_desc("file"), init("stderr"), cat(C));
opt<bool> opt_log_file_append("log-file-append", desc("append to log file"),
                              cat(C));

void closeLog() { fclose(ccls::log::file); }

} // namespace

int main(int argc, char **argv) {
  traceMe();
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  cl::SetVersionPrinter([](raw_ostream &os) {
    os << clang::getClangToolFullVersion("ccls version " CCLS_VERSION "\nclang")
       << "\n";
  });

  cl::HideUnrelatedOptions(C);

  ParseCommandLineOptions(argc, argv,
                          "C/C++/Objective-C language server\n\n"
                          "See more on https://github.com/MaskRay/ccls/wiki");

  if (opt_help) {
    PrintHelpMessage();
    return 0;
  }
  ccls::log::verbosity = ccls::log::Verbosity(opt_verbose.getValue());

  pipeline::init();
  const char *env = getenv("CCLS_CRASH_RECOVERY");
  if (!env || strcmp(env, "0") != 0)
    CrashRecoveryContext::Enable();

  bool language_server = true;

  if (opt_log_file.size()) {
    ccls::log::file =
        opt_log_file == "stderr"
            ? stderr
            : fopen(opt_log_file.c_str(), opt_log_file_append ? "ab" : "wb");
    if (!ccls::log::file) {
      fprintf(stderr, "failed to open %s\n", opt_log_file.c_str());
      return 2;
    }
    setbuf(ccls::log::file, NULL);
    atexit(closeLog);
  }

  if (opt_test_index != "!") {
    language_server = false;
    if (!ccls::runIndexTests(opt_test_index,
                             sys::Process::StandardInIsUserInput()))
      return 1;
  }

  if (language_server) {
    if (!opt_init.empty()) {
      // We check syntax error here but override client-side
      // initializationOptions in messages/initialize.cc
      g_init_options = opt_init;
      rapidjson::Document reader;
      for (const std::string &str : g_init_options) {
        rapidjson::ParseResult ok = reader.Parse(str.c_str());
        if (!ok) {
          fprintf(stderr, "Failed to parse --init as JSON: %s (%zd)\n",
                  rapidjson::GetParseError_En(ok.Code()), ok.Offset());
          return 1;
        }
        JsonReader json_reader{&reader};
        try {
          Config config;
          reflect(json_reader, config);
        } catch (std::invalid_argument &e) {
          fprintf(stderr, "Failed to parse --init %s, expected %s\n",
                  static_cast<JsonReader &>(json_reader).getPath().c_str(),
                  e.what());
          return 1;
        }
      }
    }

    sys::ChangeStdinToBinary();
    sys::ChangeStdoutToBinary();
    if (opt_index.size()) {
      SmallString<256> root(opt_index);
      sys::fs::make_absolute(root);
      pipeline::standalone(std::string(root.data(), root.size()));
    } else {
      // The thread that reads from stdin and dispatchs commands to the main
      // thread.
      pipeline::launchStdin();
      // The thread that writes responses from the main thread to stdout.
      pipeline::launchStdout();
      // Main thread which also spawns indexer threads upon the "initialize"
      // request.
      pipeline::mainLoop();
    }
  }

  return 0;
}
