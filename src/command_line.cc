// TODO: cleanup includes
#include "cache_manager.h"
#include "clang_complete.h"
#include "code_complete_cache.h"
#include "file_consumer.h"
#include "import_manager.h"
#include "import_pipeline.h"
#include "include_complete.h"
#include "indexer.h"
#include "language_server_api.h"
#include "lex_utils.h"
#include "lru_cache.h"
#include "match.h"
#include "message_handler.h"
#include "options.h"
#include "platform.h"
#include "project.h"
#include "query.h"
#include "query_utils.h"
#include "queue_manager.h"
#include "semantic_highlight_symbol_cache.h"
#include "serializer.h"
#include "standard_includes.h"
#include "test.h"
#include "threaded_queue.h"
#include "timer.h"
#include "timestamp_manager.h"
#include "work_thread.h"
#include "working_files.h"

#include <doctest/doctest.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <loguru.hpp>

#include <climits>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// TODO: provide a feature like 'https://github.com/goldsborough/clang-expand',
// ie, a fully linear view of a function with inline function calls expanded.
// We can probably use vscode decorators to achieve it.

// TODO: implement ThreadPool type which monitors CPU usage / number of work
// items per second completed and scales up/down number of running threads.

namespace {

std::vector<std::string> kEmptyArgs;

// If true stdout will be printed to stderr.
bool g_log_stdin_stdout_to_stderr = false;

// This function returns true if e2e timing should be displayed for the given
// IpcId.
bool ShouldDisplayIpcTiming(IpcId id) {
  switch (id) {
    case IpcId::TextDocumentPublishDiagnostics:
    case IpcId::CqueryPublishInactiveRegions:
    case IpcId::Unknown:
      return false;
    default:
      return true;
  }
}

REGISTER_IPC_MESSAGE(Ipc_CancelRequest);

}  // namespace

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// QUERYDB MAIN ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool QueryDbMainLoop(Config* config,
                     QueryDatabase* db,
                     MultiQueueWaiter* waiter,
                     Project* project,
                     FileConsumerSharedState* file_consumer_shared,
                     ImportManager* import_manager,
                     TimestampManager* timestamp_manager,
                     SemanticHighlightSymbolCache* semantic_cache,
                     WorkingFiles* working_files,
                     ClangCompleteManager* clang_complete,
                     IncludeComplete* include_complete,
                     CodeCompleteCache* global_code_complete_cache,
                     CodeCompleteCache* non_global_code_complete_cache,
                     CodeCompleteCache* signature_cache) {
  auto* queue = QueueManager::instance();
  bool did_work = false;

  std::vector<std::unique_ptr<BaseIpcMessage>> messages =
      queue->for_querydb.DequeueAll();
  for (auto& message : messages) {
    did_work = true;

    for (MessageHandler* handler : *MessageHandler::message_handlers) {
      if (handler->GetId() == message->method_id) {
        handler->Run(std::move(message));
        break;
      }
    }
    if (message) {
      LOG_S(FATAL) << "Exiting; unhandled IPC message "
                   << IpcIdToString(message->method_id);
      exit(1);
    }
  }

  // TODO: consider rate-limiting and checking for IPC messages so we don't
  // block requests / we can serve partial requests.

  if (QueryDb_ImportMain(config, db, import_manager, semantic_cache,
                         working_files)) {
    did_work = true;
  }

  return did_work;
}

void RunQueryDbThread(const std::string& bin_name,
                      Config* config,
                      MultiQueueWaiter* querydb_waiter,
                      MultiQueueWaiter* indexer_waiter) {
  Project project;
  SemanticHighlightSymbolCache semantic_cache;
  WorkingFiles working_files;
  FileConsumerSharedState file_consumer_shared;

  ClangCompleteManager clang_complete(
      config, &project, &working_files,
      std::bind(&EmitDiagnostics, &working_files, std::placeholders::_1,
                std::placeholders::_2),
      std::bind(&IndexWithTuFromCodeCompletion, &file_consumer_shared,
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4));

  IncludeComplete include_complete(config, &project);
  auto global_code_complete_cache = MakeUnique<CodeCompleteCache>();
  auto non_global_code_complete_cache = MakeUnique<CodeCompleteCache>();
  auto signature_cache = MakeUnique<CodeCompleteCache>();
  ImportManager import_manager;
  ImportPipelineStatus import_pipeline_status;
  TimestampManager timestamp_manager;
  QueryDatabase db;

  // Setup shared references.
  for (MessageHandler* handler : *MessageHandler::message_handlers) {
    handler->config = config;
    handler->db = &db;
    handler->waiter = indexer_waiter;
    handler->project = &project;
    handler->file_consumer_shared = &file_consumer_shared;
    handler->import_manager = &import_manager;
    handler->import_pipeline_status = &import_pipeline_status;
    handler->timestamp_manager = &timestamp_manager;
    handler->semantic_cache = &semantic_cache;
    handler->working_files = &working_files;
    handler->clang_complete = &clang_complete;
    handler->include_complete = &include_complete;
    handler->global_code_complete_cache = global_code_complete_cache.get();
    handler->non_global_code_complete_cache =
        non_global_code_complete_cache.get();
    handler->signature_cache = signature_cache.get();
  }

  // Run query db main loop.
  SetCurrentThreadName("querydb");
  while (true) {
    bool did_work = QueryDbMainLoop(
        config, &db, querydb_waiter, &project, &file_consumer_shared, &import_manager,
        &timestamp_manager, &semantic_cache, &working_files, &clang_complete,
        &include_complete, global_code_complete_cache.get(),
        non_global_code_complete_cache.get(), signature_cache.get());

    // Cleanup and free any unused memory.
    FreeUnusedMemory();

    if (!did_work) {
      auto* queue = QueueManager::instance();
      querydb_waiter->Wait(&queue->on_indexed, &queue->for_querydb,
                           &queue->do_id_map);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// STDIN MAIN //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Separate thread whose only job is to read from stdin and
// dispatch read commands to the actual indexer program. This
// cannot be done on the main thread because reading from std::cin
// blocks.
//
// |ipc| is connected to a server.
void LaunchStdinLoop(Config* config,
                     std::unordered_map<IpcId, Timer>* request_times) {
  // If flushing cin requires flushing cout there could be deadlocks in some
  // clients.
  std::cin.tie(nullptr);

  WorkThread::StartThread("stdin", [request_times]() {
    auto* queue = QueueManager::instance();
    while (true) {
      std::unique_ptr<BaseIpcMessage> message =
          MessageRegistry::instance()->ReadMessageFromStdin(
              g_log_stdin_stdout_to_stderr);

      // Message parsing can fail if we don't recognize the method.
      if (!message)
        continue;

      // Cache |method_id| so we can access it after moving |message|.
      IpcId method_id = message->method_id;

      (*request_times)[message->method_id] = Timer();

      switch (method_id) {
        case IpcId::Initialized: {
          // TODO: don't send output until we get this notification
          break;
        }

        case IpcId::CancelRequest: {
          // TODO: support cancellation
          break;
        }

        case IpcId::Exit:
        case IpcId::Initialize:
        case IpcId::TextDocumentDidOpen:
        case IpcId::CqueryTextDocumentDidView:
        case IpcId::TextDocumentDidChange:
        case IpcId::TextDocumentDidClose:
        case IpcId::TextDocumentDidSave:
        case IpcId::TextDocumentFormatting:
        case IpcId::TextDocumentRangeFormatting:
        case IpcId::TextDocumentOnTypeFormatting:
        case IpcId::TextDocumentRename:
        case IpcId::TextDocumentCompletion:
        case IpcId::TextDocumentSignatureHelp:
        case IpcId::TextDocumentDefinition:
        case IpcId::TextDocumentDocumentHighlight:
        case IpcId::TextDocumentHover:
        case IpcId::TextDocumentReferences:
        case IpcId::TextDocumentDocumentSymbol:
        case IpcId::TextDocumentDocumentLink:
        case IpcId::TextDocumentCodeAction:
        case IpcId::TextDocumentCodeLens:
        case IpcId::WorkspaceSymbol:
        case IpcId::CqueryFreshenIndex:
        case IpcId::CqueryTypeHierarchyTree:
        case IpcId::CqueryCallTreeInitial:
        case IpcId::CqueryCallTreeExpand:
        case IpcId::CqueryVars:
        case IpcId::CqueryCallers:
        case IpcId::CqueryBase:
        case IpcId::CqueryDerived:
        case IpcId::CqueryIndexFile:
        case IpcId::CqueryWait: {
          queue->for_querydb.Enqueue(std::move(message));
          break;
        }

        default: {
          LOG_S(ERROR) << "Unhandled IPC message " << IpcIdToString(method_id);
          exit(1);
        }
      }

      // If the message was to exit then querydb will take care of the actual
      // exit. Stop reading from stdin since it might be detached.
      if (method_id == IpcId::Exit)
        break;
    }
  });
}

void LaunchStdoutThread(std::unordered_map<IpcId, Timer>* request_times,
                        MultiQueueWaiter* waiter) {
  WorkThread::StartThread("stdout", [=]() {
    auto* queue = QueueManager::instance();

    while (true) {
      std::vector<Stdout_Request> messages = queue->for_stdout.DequeueAll();
      if (messages.empty()) {
        waiter->Wait(&queue->for_stdout);
        continue;
      }

      for (auto& message : messages) {
        if (ShouldDisplayIpcTiming(message.id)) {
          Timer time = (*request_times)[message.id];
          time.ResetAndPrint("[e2e] Running " +
                             std::string(IpcIdToString(message.id)));
        }

        if (g_log_stdin_stdout_to_stderr) {
          std::ostringstream sstream;
          sstream << "[COUT] |";
          sstream << message.content;
          sstream << "|\n";
          std::cerr << sstream.str();
          std::cerr.flush();
        }

        std::cout << message.content;
        std::cout.flush();
      }
    }
  });
}

void LanguageServerMain(const std::string& bin_name,
                        Config* config,
                        MultiQueueWaiter* querydb_waiter,
                        MultiQueueWaiter* indexer_waiter,
                        MultiQueueWaiter* stdout_waiter) {
  std::unordered_map<IpcId, Timer> request_times;

  LaunchStdinLoop(config, &request_times);

  // We run a dedicated thread for writing to stdout because there can be an
  // unknown number of delays when output information.
  LaunchStdoutThread(&request_times, stdout_waiter);

  // Start querydb which takes over this thread. The querydb will launch
  // indexer threads as needed.
  RunQueryDbThread(bin_name, config, querydb_waiter, indexer_waiter);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// MAIN ////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  TraceMe();

  std::unordered_map<std::string, std::string> options =
      ParseOptions(argc, argv);

  if (!HasOption(options, "--log-all-to-stderr"))
    loguru::g_stderr_verbosity = loguru::Verbosity_WARNING;

  loguru::g_flush_interval_ms = 0;
  loguru::init(argc, argv);

  MultiQueueWaiter querydb_waiter, indexer_waiter, stdout_waiter;
  QueueManager::CreateInstance(&querydb_waiter, &indexer_waiter,
                               &stdout_waiter);

  // bool loop = true;
  // while (loop)
  //  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  // std::this_thread::sleep_for(std::chrono::seconds(10));

  PlatformInit();
  IndexInit();

  bool print_help = true;

  if (HasOption(options, "--log-file")) {
    loguru::add_file(options["--log-file"].c_str(), loguru::Truncate,
                     loguru::Verbosity_MAX);
  }

  if (HasOption(options, "--log-stdin-stdout-to-stderr"))
    g_log_stdin_stdout_to_stderr = true;

  if (HasOption(options, "--clang-sanity-check")) {
    print_help = false;
    ClangSanityCheck();
  }

  if (HasOption(options, "--test-unit")) {
    print_help = false;
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();
    if (res != 0 || context.shouldExit())
      return res;
  }

  if (HasOption(options, "--test-index")) {
    print_help = false;
    if (!RunIndexTests(options["--test-index"], !HasOption(options, "--ci")))
      return -1;
  }

  if (HasOption(options, "--enable-comments")) {
    // TODO Place this global variable into config
    extern bool g_enable_comments;
    g_enable_comments = true;
  }

  if (HasOption(options, "--language-server")) {
    print_help = false;
    // std::cerr << "Running language server" << std::endl;
    auto config = MakeUnique<Config>();
    LanguageServerMain(argv[0], config.get(), &querydb_waiter, &indexer_waiter,
                       &stdout_waiter);
    return 0;
  }

  if (HasOption(options, "--wait-for-input")) {
    std::cerr << std::endl << "[Enter] to exit" << std::endl;
    std::cin.get();
  }

  if (print_help) {
    std::cout
        << R"help(cquery is a low-latency C/C++/Objective-C language server.

Command line options:
  --language-server
                Run as a language server. This implements the language server
                spec over STDIN and STDOUT.
  --test-unit   Run unit tests.
  --test-index <opt_filter_path>
                Run index tests. opt_filter_path can be used to specify which
                test to run (ie, "foo" will run all tests which contain "foo"
                in the path). If not provided all tests are run.
  --log-stdin-stdout-to-stderr
                Print stdin and stdout messages to stderr. This is a aid for
                developing new language clients, as it makes it easier to figure
                out how the client is interacting with cquery.
  --log-file <absoulte_path>
                Emit diagnostic logging to the given path, which is taken
                literally, ie, it will be relative to the current working
                directory.
  --log-all-to-stderr
                Write all log messages to STDERR.
  --clang-sanity-check
                Run a simple index test. Verifies basic clang functionality.
                Needs to be executed from the cquery root checkout directory.
  --wait-for-input
                If true, cquery will wait for an '[Enter]' before exiting.
                Useful on windows.
  --help        Print this help information.
  --ci          Prevents tests from prompting the user for input. Used for
                continuous integration so it can fail faster instead of timing
                out.

Configuration:
  When opening up a directory, cquery will look for a compile_commands.json file
  emitted by your preferred build system. If not present, cquery will use a
  recursive directory listing instead. Command line flags can be provided by
  adding a file named `.cquery` in the top-level directory. Each line in that
  file is a separate argument.

  There are also a number of configuration options available when initializing
  the language server - your editor should have tooling to describe those
  options.  See |Config| in this source code for a detailed list of all
  currently supported options.
)help";
  }

  return 0;
}
