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
#include "serializers/json.h"
#include "test.h"
#include "threaded_queue.h"
#include "timer.h"
#include "timestamp_manager.h"
#include "work_thread.h"
#include "working_files.h"

#include <doctest/doctest.h>
#include <rapidjson/error/en.h>
#include <loguru.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// TODO: provide a feature like 'https://github.com/goldsborough/clang-expand',
// ie, a fully linear view of a function with inline function calls expanded.
// We can probably use vscode decorators to achieve it.

// TODO: implement ThreadPool type which monitors CPU usage / number of work
// items per second completed and scales up/down number of running threads.

std::string g_init_options;
bool g_debug;

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

void PrintHelp() {
  std::cout << R"help(cquery is a low-latency C/C++/Objective-C language server.

Mode:
  --clang-sanity-check
                Run a simple index test. Verifies basic clang functionality.
                Needs to be executed from the cquery root checkout directory.
  --test-unit   Run unit tests.
  --test-index <opt_filter_path>
                Run index tests. opt_filter_path can be used to specify which
                test to run (ie, "foo" will run all tests which contain "foo"
                in the path). If not provided all tests are run.
  (default if no other mode is specified)
                Run as a language server over stdin and stdout

Other command line options:
  --debug       Disable libclang crash recovery so that in case of libclang or
                indexer callback issue, the process will crash and we can
                get a stack trace.
  --init <initializationOptions>
                Override client provided initialization options
         https://github.com/cquery-project/cquery/wiki/Initialization-options
  --log-stdin-stdout-to-stderr
                Print stdin (requests) and stdout (responses) to stderr
  --log-file <path>    Logging file for diagnostics
  --log-all-to-stderr  Write all log messages to STDERR.
  --wait-for-input     Wait for an '[Enter]' before exiting
  --help        Print this help information.
  --ci          Prevents tests from prompting the user for input. Used for
                continuous integration so it can fail faster instead of timing
                out.

See more on https://github.com/cquery-project/cquery/wiki
)help";
}

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
                     ImportPipelineStatus* status,
                     TimestampManager* timestamp_manager,
                     SemanticHighlightSymbolCache* semantic_cache,
                     WorkingFiles* working_files,
                     ClangCompleteManager* clang_complete,
                     IncludeComplete* include_complete,
                     CodeCompleteCache* global_code_complete_cache,
                     CodeCompleteCache* non_global_code_complete_cache,
                     CodeCompleteCache* signature_cache) {
  auto* queue = QueueManager::instance();
  std::vector<std::unique_ptr<BaseIpcMessage>> messages =
      queue->for_querydb.DequeueAll();
  bool did_work = messages.size();
  for (auto& message : messages) {
    QueryDb_Handle(message);
    if (message) {
      LOG_S(FATAL) << "Exiting; unhandled IPC message "
                   << IpcIdToString(message->method_id);
      exit(1);
    }
  }

  // TODO: consider rate-limiting and checking for IPC messages so we don't
  // block requests / we can serve partial requests.

  if (QueryDb_ImportMain(config, db, import_manager, status, semantic_cache,
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
      [&](std::string path, std::vector<lsDiagnostic> diagnostics) {
        EmitDiagnostics(&working_files, path, diagnostics);
      },
      [&](ClangTranslationUnit* tu, const std::vector<CXUnsavedFile>& unsaved,
          const std::string& path, const std::vector<std::string>& args) {
        IndexWithTuFromCodeCompletion(config, &file_consumer_shared, tu,
                                      unsaved, path, args);
      });

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
        config, &db, querydb_waiter, &project, &file_consumer_shared,
        &import_manager, &import_pipeline_status, &timestamp_manager,
        &semantic_cache, &working_files, &clang_complete, &include_complete,
        global_code_complete_cache.get(), non_global_code_complete_cache.get(),
        signature_cache.get());

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
      std::unique_ptr<BaseIpcMessage> message;
      optional<std::string> err =
          MessageRegistry::instance()->ReadMessageFromStdin(
              g_log_stdin_stdout_to_stderr, &message);

      // Message parsing can fail if we don't recognize the method.
      if (err) {
        // The message may be partially deserialized.
        // Emit an error ResponseMessage if |id| is available.
        if (message) {
          lsRequestId id = message->GetRequestId();
          if (!std::holds_alternative<std::monostate>(id)) {
            Out_Error out;
            out.id = id;
            out.error.code = lsErrorCodes::InvalidParams;
            out.error.message = std::move(*err);
            queue->WriteStdout(IpcId::Unknown, out);
          }
        }
        continue;
      }

      // Cache |method_id| so we can access it after moving |message|.
      IpcId method_id = message->method_id;
      (*request_times)[method_id] = Timer();

      switch (method_id) {
        case IpcId::Initialized: {
          // TODO: don't send output until we get this notification
          break;
        }

        case IpcId::CancelRequest: {
          // TODO: support cancellation
          break;
        }

        case IpcId::Shutdown:
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
        case IpcId::WorkspaceDidChangeWatchedFiles:
        case IpcId::WorkspaceSymbol:
        case IpcId::CqueryFreshenIndex:
        case IpcId::CqueryTypeHierarchyTree:
        case IpcId::CqueryCallTreeInitial:
        case IpcId::CqueryCallTreeExpand:
        case IpcId::CqueryMemberHierarchyInitial:
        case IpcId::CqueryMemberHierarchyExpand:
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
          std::cerr << "[COUT] |" << message.content << "|\n";
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

  if (HasOption(options, "-h") || HasOption(options, "--help")) {
    PrintHelp();
    return 0;
  }

  if (!HasOption(options, "--log-all-to-stderr"))
    loguru::g_stderr_verbosity = loguru::Verbosity_WARNING;

  loguru::g_flush_interval_ms = 0;
  loguru::init(argc, argv);

  MultiQueueWaiter querydb_waiter, indexer_waiter, stdout_waiter;
  QueueManager::CreateInstance(&querydb_waiter, &indexer_waiter,
                               &stdout_waiter);

  PlatformInit();
  g_debug = HasOption(options, "--debug");
  IndexInit();

  bool language_server = true;

  if (HasOption(options, "--log-file")) {
    loguru::add_file(options["--log-file"].c_str(), loguru::Truncate,
                     loguru::Verbosity_MAX);
  }

  if (HasOption(options, "--log-stdin-stdout-to-stderr"))
    g_log_stdin_stdout_to_stderr = true;

  if (HasOption(options, "--clang-sanity-check")) {
    language_server = false;
    ClangSanityCheck();
  }

  if (HasOption(options, "--test-unit")) {
    g_debug = true;
    language_server = false;
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();
    if (res != 0 || context.shouldExit())
      return res;
  }

  if (HasOption(options, "--test-index")) {
    g_debug = true;
    language_server = false;
    if (!RunIndexTests(options["--test-index"], !HasOption(options, "--ci")))
      return 1;
  }

  if (language_server) {
    if (HasOption(options, "--init")) {
      // We check syntax error here but override client-side
      // initializationOptions in messages/initialize.cc
      g_init_options = options["--init"];
      rapidjson::Document reader;
      rapidjson::ParseResult ok = reader.Parse(g_init_options.c_str());
      if (!ok) {
        std::cerr << "Failed to parse --init as JSON: "
                  << rapidjson::GetParseError_En(ok.Code()) << " ("
                  << ok.Offset() << ")\n";
        return 1;
      }
      JsonReader json_reader{&reader};
      try {
        Config config;
        Reflect(json_reader, config);
      } catch (std::invalid_argument& e) {
        std::cerr << "Fail to parse --init "
                  << static_cast<JsonReader&>(json_reader).GetPath()
                  << ", expected " << e.what() << "\n";
        return 1;
      }
    }

    // std::cerr << "Running language server" << std::endl;
    auto config = MakeUnique<Config>();
    LanguageServerMain(argv[0], config.get(), &querydb_waiter, &indexer_waiter,
                       &stdout_waiter);
  }

  if (HasOption(options, "--wait-for-input")) {
    std::cerr << std::endl << "[Enter] to exit" << std::endl;
    getchar();
  }

  return 0;
}
