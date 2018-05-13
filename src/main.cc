#include "import_pipeline.h"
#include "platform.h"
#include "serializer.h"
#include "serializers/json.h"
#include "test.h"
#include "working_files.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Process.h>
using namespace llvm;
using namespace llvm::cl;

#include <doctest/doctest.h>
#include <rapidjson/error/en.h>
#include <loguru.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>

std::string g_init_options;

namespace {
opt<bool> opt_help("h", desc("Alias for -help"));
opt<int> opt_verbose("v", desc("verbosity"), init(0));
opt<bool> opt_test_index("test-index", desc("run index tests"));
opt<bool> opt_test_unit("test-unit", desc("run unit tests"));

opt<std::string> opt_init("init", desc("extra initialization options"));
opt<std::string> opt_log_file("log-file", desc("log"), value_desc("filename"));
opt<std::string> opt_log_file_append("log-file-append", desc("log"), value_desc("filename"));

list<std::string> opt_extra(Positional, ZeroOrMore, desc("extra"));

}  // namespace

int main(int argc, char** argv) {
  TraceMe();

  ParseCommandLineOptions(argc, argv,
                          "C/C++/Objective-C language server\n\n"
                          "See more on https://github.com/MaskRay/ccls/wiki");

  if (opt_help) {
    PrintHelpMessage();
    // Also emit doctest help if --test-unit is passed.
    if (!opt_test_unit)
      return 0;
  }

  loguru::g_stderr_verbosity = opt_verbose - 1;
  loguru::g_preamble_date = false;
  loguru::g_preamble_time = false;
  loguru::g_preamble_verbose = false;
  loguru::g_flush_interval_ms = 0;
  loguru::init(argc, argv);

  MultiQueueWaiter querydb_waiter, indexer_waiter, stdout_waiter;
  QueueManager::Init(&querydb_waiter, &indexer_waiter, &stdout_waiter);

#ifdef _WIN32
  // We need to write to stdout in binary mode because in Windows, writing
  // \n will implicitly write \r\n. Language server API will ignore a
  // \r\r\n split request.
  _setmode(_fileno(stdout), O_BINARY);
  _setmode(_fileno(stdin), O_BINARY);
#endif

  IndexInit();

  bool language_server = true;

  if (opt_log_file.size())
    loguru::add_file(opt_log_file.c_str(), loguru::Truncate, opt_verbose);
  if (opt_log_file_append.size())
    loguru::add_file(opt_log_file_append.c_str(), loguru::Append, opt_verbose);

  if (opt_test_unit) {
    language_server = false;
    doctest::Context context;
    std::vector<const char*> args{argv[0]};
    if (opt_help)
      args.push_back("-h");
    for (auto& arg : opt_extra)
      args.push_back(arg.c_str());
    context.applyCommandLine(args.size(), args.data());
    int res = context.run();
    if (res != 0 || context.shouldExit())
      return res;
  }

  if (opt_test_index) {
    language_server = false;
    if (!RunIndexTests("", sys::Process::StandardInIsUserInput()))
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
      } catch (std::invalid_argument& e) {
        fprintf(stderr, "Failed to parse --init %s, expected %s\n",
                static_cast<JsonReader&>(json_reader).GetPath().c_str(),
                e.what());
        return 1;
      }
    }

    std::unordered_map<MethodType, Timer> request_times;

    // The thread that reads from stdin and dispatchs commands to the main thread.
    LaunchStdinLoop(&request_times);
    // The thread that writes responses from the main thread to stdout.
    LaunchStdoutThread(&request_times, &stdout_waiter);
    // Main thread which also spawns indexer threads upon the "initialize" request.
    MainLoop(&querydb_waiter, &indexer_waiter);
  }

  return 0;
}
