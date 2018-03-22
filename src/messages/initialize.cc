#include "cache_manager.h"
#include "diagnostics_engine.h"
#include "import_pipeline.h"
#include "include_complete.h"
#include "message_handler.h"
#include "platform.h"
#include "project.h"
#include "queue_manager.h"
#include "semantic_highlight_symbol_cache.h"
#include "serializers/json.h"
#include "timer.h"
#include "work_thread.h"
#include "working_files.h"

#include <loguru.hpp>

#include <iostream>
#include <stdexcept>
#include <thread>

// TODO Cleanup global variables
extern std::string g_init_options;

namespace {

MethodType kMethodType = "initialize";

// Code Lens options.
struct lsCodeLensOptions {
  // Code lens has a resolve provider as well.
  bool resolveProvider = false;
};
MAKE_REFLECT_STRUCT(lsCodeLensOptions, resolveProvider);

// Completion options.
struct lsCompletionOptions {
  // The server provides support to resolve additional
  // information for a completion item.
  bool resolveProvider = false;

  // The characters that trigger completion automatically.
  // vscode doesn't support trigger character sequences, so we use ':'
  // for
  // '::' and '>' for '->'. See
  // https://github.com/Microsoft/language-server-protocol/issues/138.
  std::vector<std::string> triggerCharacters = {".", ":",  ">", "#",
                                                "<", "\"", "/"};
};
MAKE_REFLECT_STRUCT(lsCompletionOptions, resolveProvider, triggerCharacters);

// Format document on type options
struct lsDocumentOnTypeFormattingOptions {
  // A character on which formatting should be triggered, like `}`.
  std::string firstTriggerCharacter;

  // More trigger characters.
  std::vector<std::string> moreTriggerCharacter;
};
MAKE_REFLECT_STRUCT(lsDocumentOnTypeFormattingOptions,
                    firstTriggerCharacter,
                    moreTriggerCharacter);

// Document link options
struct lsDocumentLinkOptions {
  // Document links have a resolve provider as well.
  bool resolveProvider = true;
};
MAKE_REFLECT_STRUCT(lsDocumentLinkOptions, resolveProvider);

// Execute command options.
struct lsExecuteCommandOptions {
  // The commands to be executed on the server
  std::vector<std::string> commands;
};
MAKE_REFLECT_STRUCT(lsExecuteCommandOptions, commands);

// Save options.
struct lsSaveOptions {
  // The client is supposed to include the content on save.
  bool includeText = false;
};
MAKE_REFLECT_STRUCT(lsSaveOptions, includeText);

// Signature help options.
struct lsSignatureHelpOptions {
  // The characters that trigger signature help automatically.
  // NOTE: If updating signature help tokens make sure to also update
  // WorkingFile::FindClosestCallNameInBuffer.
  std::vector<std::string> triggerCharacters = {"(", ","};
};
MAKE_REFLECT_STRUCT(lsSignatureHelpOptions, triggerCharacters);

// Defines how the host (editor) should sync document changes to the language
// server.
enum class lsTextDocumentSyncKind {
  // Documents should not be synced at all.
  None = 0,

  // Documents are synced by always sending the full content
  // of the document.
  Full = 1,

  // Documents are synced by sending the full content on open.
  // After that only incremental updates to the document are
  // send.
  Incremental = 2
};
MAKE_REFLECT_TYPE_PROXY(lsTextDocumentSyncKind)

struct lsTextDocumentSyncOptions {
  // Open and close notifications are sent to the server.
  bool openClose = false;
  // Change notificatins are sent to the server. See TextDocumentSyncKind.None,
  // TextDocumentSyncKind.Full and TextDocumentSyncKindIncremental.
  lsTextDocumentSyncKind change = lsTextDocumentSyncKind::Incremental;
  // Will save notifications are sent to the server.
  optional<bool> willSave;
  // Will save wait until requests are sent to the server.
  optional<bool> willSaveWaitUntil;
  // Save notifications are sent to the server.
  optional<lsSaveOptions> save;
};
MAKE_REFLECT_STRUCT(lsTextDocumentSyncOptions,
                    openClose,
                    change,
                    willSave,
                    willSaveWaitUntil,
                    save);

struct lsServerCapabilities {
  // Defines how text documents are synced. Is either a detailed structure
  // defining each notification or for backwards compatibility the
  // TextDocumentSyncKind number.
  // TODO: It seems like the new API is broken and doesn't work.
  // optional<lsTextDocumentSyncOptions> textDocumentSync;
  lsTextDocumentSyncKind textDocumentSync = lsTextDocumentSyncKind::Incremental;

  // The server provides hover support.
  bool hoverProvider = true;
  // The server provides completion support.
  lsCompletionOptions completionProvider;
  // The server provides signature help support.
  lsSignatureHelpOptions signatureHelpProvider;
  // The server provides goto definition support.
  bool definitionProvider = true;
  // The server provides Goto Type Definition support.
  bool typeDefinitionProvider = true;
  // The server provides find references support.
  bool referencesProvider = true;
  // The server provides document highlight support.
  bool documentHighlightProvider = true;
  // The server provides document symbol support.
  bool documentSymbolProvider = true;
  // The server provides workspace symbol support.
  bool workspaceSymbolProvider = true;
  // The server provides code actions.
  bool codeActionProvider = true;
  // The server provides code lens.
  lsCodeLensOptions codeLensProvider;
  // The server provides document formatting.
  bool documentFormattingProvider = false;
  // The server provides document range formatting.
  bool documentRangeFormattingProvider = false;
  // The server provides document formatting on typing.
  optional<lsDocumentOnTypeFormattingOptions> documentOnTypeFormattingProvider;
  // The server provides rename support.
  bool renameProvider = true;
  // The server provides document link support.
  lsDocumentLinkOptions documentLinkProvider;
  // The server provides execute command support.
  lsExecuteCommandOptions executeCommandProvider;
};
MAKE_REFLECT_STRUCT(lsServerCapabilities,
                    textDocumentSync,
                    hoverProvider,
                    completionProvider,
                    signatureHelpProvider,
                    definitionProvider,
                    referencesProvider,
                    documentHighlightProvider,
                    documentSymbolProvider,
                    workspaceSymbolProvider,
                    codeActionProvider,
                    codeLensProvider,
                    documentFormattingProvider,
                    documentRangeFormattingProvider,
                    documentOnTypeFormattingProvider,
                    renameProvider,
                    documentLinkProvider,
                    executeCommandProvider);

// Workspace specific client capabilities.
struct lsWorkspaceClientCapabilites {
  // The client supports applying batch edits to the workspace.
  optional<bool> applyEdit;

  struct lsWorkspaceEdit {
    // The client supports versioned document changes in `WorkspaceEdit`s
    optional<bool> documentChanges;
  };

  // Capabilities specific to `WorkspaceEdit`s
  optional<lsWorkspaceEdit> workspaceEdit;

  struct lsGenericDynamicReg {
    // Did foo notification supports dynamic registration.
    optional<bool> dynamicRegistration;
  };

  // Capabilities specific to the `workspace/didChangeConfiguration`
  // notification.
  optional<lsGenericDynamicReg> didChangeConfiguration;

  // Capabilities specific to the `workspace/didChangeWatchedFiles`
  // notification.
  optional<lsGenericDynamicReg> didChangeWatchedFiles;

  // Capabilities specific to the `workspace/symbol` request.
  optional<lsGenericDynamicReg> symbol;

  // Capabilities specific to the `workspace/executeCommand` request.
  optional<lsGenericDynamicReg> executeCommand;
};

MAKE_REFLECT_STRUCT(lsWorkspaceClientCapabilites::lsWorkspaceEdit,
                    documentChanges);
MAKE_REFLECT_STRUCT(lsWorkspaceClientCapabilites::lsGenericDynamicReg,
                    dynamicRegistration);
MAKE_REFLECT_STRUCT(lsWorkspaceClientCapabilites,
                    applyEdit,
                    workspaceEdit,
                    didChangeConfiguration,
                    didChangeWatchedFiles,
                    symbol,
                    executeCommand);

// Text document specific client capabilities.
struct lsTextDocumentClientCapabilities {
  struct lsSynchronization {
    // Whether text document synchronization supports dynamic registration.
    optional<bool> dynamicRegistration;

    // The client supports sending will save notifications.
    optional<bool> willSave;

    // The client supports sending a will save request and
    // waits for a response providing text edits which will
    // be applied to the document before it is saved.
    optional<bool> willSaveWaitUntil;

    // The client supports did save notifications.
    optional<bool> didSave;
  };

  lsSynchronization synchronization;

  struct lsCompletion {
    // Whether completion supports dynamic registration.
    optional<bool> dynamicRegistration;

    struct lsCompletionItem {
      // Client supports snippets as insert text.
      //
      // A snippet can define tab stops and placeholders with `$1`, `$2`
      // and `${3:foo}`. `$0` defines the final tab stop, it defaults to
      // the end of the snippet. Placeholders with equal identifiers are linked,
      // that is typing in one will update others too.
      optional<bool> snippetSupport;
    };

    // The client supports the following `CompletionItem` specific
    // capabilities.
    optional<lsCompletionItem> completionItem;
  };
  // Capabilities specific to the `textDocument/completion`
  optional<lsCompletion> completion;

  struct lsGenericDynamicReg {
    // Whether foo supports dynamic registration.
    optional<bool> dynamicRegistration;
  };

  // Capabilities specific to the `textDocument/hover`
  optional<lsGenericDynamicReg> hover;

  // Capabilities specific to the `textDocument/signatureHelp`
  optional<lsGenericDynamicReg> signatureHelp;

  // Capabilities specific to the `textDocument/references`
  optional<lsGenericDynamicReg> references;

  // Capabilities specific to the `textDocument/documentHighlight`
  optional<lsGenericDynamicReg> documentHighlight;

  // Capabilities specific to the `textDocument/documentSymbol`
  optional<lsGenericDynamicReg> documentSymbol;

  // Capabilities specific to the `textDocument/formatting`
  optional<lsGenericDynamicReg> formatting;

  // Capabilities specific to the `textDocument/rangeFormatting`
  optional<lsGenericDynamicReg> rangeFormatting;

  // Capabilities specific to the `textDocument/onTypeFormatting`
  optional<lsGenericDynamicReg> onTypeFormatting;

  // Capabilities specific to the `textDocument/definition`
  optional<lsGenericDynamicReg> definition;

  // Capabilities specific to the `textDocument/codeAction`
  optional<lsGenericDynamicReg> codeAction;

  struct CodeLensRegistrationOptions : public lsGenericDynamicReg {
    // Code lens has a resolve provider as well.
    bool resolveProvider;
  };

  // Capabilities specific to the `textDocument/codeLens`
  optional<CodeLensRegistrationOptions> codeLens;

  // Capabilities specific to the `textDocument/documentLink`
  optional<lsGenericDynamicReg> documentLink;

  // Capabilities specific to the `textDocument/rename`
  optional<lsGenericDynamicReg> rename;
};

MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsSynchronization,
                    dynamicRegistration,
                    willSave,
                    willSaveWaitUntil,
                    didSave);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsCompletion,
                    dynamicRegistration,
                    completionItem);
MAKE_REFLECT_STRUCT(
    lsTextDocumentClientCapabilities::lsCompletion::lsCompletionItem,
    snippetSupport);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsGenericDynamicReg,
                    dynamicRegistration);
MAKE_REFLECT_STRUCT(
    lsTextDocumentClientCapabilities::CodeLensRegistrationOptions,
    dynamicRegistration,
    resolveProvider);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities,
                    synchronization,
                    completion,
                    hover,
                    signatureHelp,
                    references,
                    documentHighlight,
                    documentSymbol,
                    formatting,
                    rangeFormatting,
                    onTypeFormatting,
                    definition,
                    codeAction,
                    codeLens,
                    documentLink,
                    rename);

struct lsClientCapabilities {
  // Workspace specific client capabilities.
  optional<lsWorkspaceClientCapabilites> workspace;

  // Text document specific client capabilities.
  optional<lsTextDocumentClientCapabilities> textDocument;

  /**
   * Experimental client capabilities.
   */
  // experimental?: any; // TODO
};
MAKE_REFLECT_STRUCT(lsClientCapabilities, workspace, textDocument);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////// INITIALIZATION ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct lsInitializeParams {
  // The process Id of the parent process that started
  // the server. Is null if the process has not been started by another process.
  // If the parent process is not alive then the server should exit (see exit
  // notification) its process.
  optional<int> processId;

  // The rootPath of the workspace. Is null
  // if no folder is open.
  //
  // @deprecated in favour of rootUri.
  optional<std::string> rootPath;

  // The rootUri of the workspace. Is null if no
  // folder is open. If both `rootPath` and `rootUri` are set
  // `rootUri` wins.
  optional<lsDocumentUri> rootUri;

  // User provided initialization options.
  optional<Config> initializationOptions;

  // The capabilities provided by the client (editor or tool)
  lsClientCapabilities capabilities;

  enum class lsTrace {
    // NOTE: serialized as a string, one of 'off' | 'messages' | 'verbose';
    Off,       // off
    Messages,  // messages
    Verbose    // verbose
  };

  // The initial trace setting. If omitted trace is disabled ('off').
  lsTrace trace = lsTrace::Off;
};

void Reflect(Reader& reader, lsInitializeParams::lsTrace& value) {
  if (!reader.IsString()) {
    value = lsInitializeParams::lsTrace::Off;
    return;
  }
  std::string v = reader.GetString();
  if (v == "off")
    value = lsInitializeParams::lsTrace::Off;
  else if (v == "messages")
    value = lsInitializeParams::lsTrace::Messages;
  else if (v == "verbose")
    value = lsInitializeParams::lsTrace::Verbose;
}

#if 0  // unused
void Reflect(Writer& writer, lsInitializeParams::lsTrace& value) {
  switch (value) {
    case lsInitializeParams::lsTrace::Off:
      writer.String("off");
      break;
    case lsInitializeParams::lsTrace::Messages:
      writer.String("messages");
      break;
    case lsInitializeParams::lsTrace::Verbose:
      writer.String("verbose");
      break;
  }
}
#endif

MAKE_REFLECT_STRUCT(lsInitializeParams,
                    processId,
                    rootPath,
                    rootUri,
                    initializationOptions,
                    capabilities,
                    trace);

struct lsInitializeError {
  // Indicates whether the client should retry to send the
  // initilize request after showing the message provided
  // in the ResponseError.
  bool retry;
};
MAKE_REFLECT_STRUCT(lsInitializeError, retry);

struct In_InitializeRequest : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  lsInitializeParams params;
};
MAKE_REFLECT_STRUCT(In_InitializeRequest, id, params);
REGISTER_IN_MESSAGE(In_InitializeRequest);

struct Out_InitializeResponse : public lsOutMessage<Out_InitializeResponse> {
  struct InitializeResult {
    lsServerCapabilities capabilities;
  };
  lsRequestId id;
  InitializeResult result;
};
MAKE_REFLECT_STRUCT(Out_InitializeResponse::InitializeResult, capabilities);
MAKE_REFLECT_STRUCT(Out_InitializeResponse, jsonrpc, id, result);

struct Handler_Initialize : BaseMessageHandler<In_InitializeRequest> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_InitializeRequest* request) override {
    // Log initialization parameters.
    rapidjson::StringBuffer output;
    rapidjson::Writer<rapidjson::StringBuffer> writer(output);
    JsonWriter json_writer(&writer);
    Reflect(json_writer, request->params.initializationOptions);
    LOG_S(INFO) << "Init parameters: " << output.GetString();

    if (request->params.rootUri) {
      std::string project_path =
          NormalizePath(request->params.rootUri->GetPath());
      LOG_S(INFO) << "[querydb] Initialize in directory " << project_path
                  << " with uri " << request->params.rootUri->raw_uri;

      {
        if (request->params.initializationOptions)
          *config = *request->params.initializationOptions;
        else
          *config = Config();
        rapidjson::Document reader;
        reader.Parse(g_init_options.c_str());
        if (!reader.HasParseError()) {
          JsonReader json_reader{&reader};
          try {
            Reflect(json_reader, *config);
          } catch (std::invalid_argument&) {
            // This will not trigger because parse error is handled in
            // MessageRegistry::Parse in lsp.cc
          }
        }

        if (config->cacheDirectory.empty()) {
          LOG_S(ERROR) << "cacheDirectory cannot be empty.";
          exit(1);
        } else {
          config->cacheDirectory = NormalizePath(config->cacheDirectory);
          EnsureEndsInSlash(config->cacheDirectory);
        }
      }

      // Client capabilities
      if (request->params.capabilities.textDocument) {
        const auto& cap = *request->params.capabilities.textDocument;
        if (cap.completion && cap.completion->completionItem)
          config->client.snippetSupport =
              cap.completion->completionItem->snippetSupport.value_or(false);
      }

      // Check client version.
      if (config->clientVersion.has_value() &&
          *config->clientVersion != kExpectedClientVersion) {
        Out_ShowLogMessage out;
        out.display_type = Out_ShowLogMessage::DisplayType::Show;
        out.params.type = lsMessageType::Error;
        out.params.message =
            "cquery client (v" + std::to_string(*config->clientVersion) +
            ") and server (v" + std::to_string(kExpectedClientVersion) +
            ") version mismatch. Please update ";
        if (config->clientVersion > kExpectedClientVersion)
          out.params.message += "the cquery binary.";
        else
          out.params.message +=
              "your extension client (VSIX file). Make sure to uninstall "
              "the cquery extension and restart vscode before "
              "reinstalling.";
        out.Write(std::cout);
      }

      // Ensure there is a resource directory.
      if (config->resourceDirectory.empty())
        config->resourceDirectory = GetDefaultResourceDirectory();
      LOG_S(INFO) << "Using -resource-dir=" << config->resourceDirectory;

      // Send initialization before starting indexers, so we don't send a
      // status update too early.
      // TODO: query request->params.capabilities.textDocument and support
      // only things the client supports.

      Out_InitializeResponse out;
      out.id = request->id;

      // out.result.capabilities.textDocumentSync =
      // lsTextDocumentSyncOptions();
      // out.result.capabilities.textDocumentSync->openClose = true;
      // out.result.capabilities.textDocumentSync->change =
      // lsTextDocumentSyncKind::Full;
      // out.result.capabilities.textDocumentSync->willSave = true;
      // out.result.capabilities.textDocumentSync->willSaveWaitUntil =
      // true;

#if USE_CLANG_CXX
      out.result.capabilities.documentFormattingProvider = true;
      out.result.capabilities.documentRangeFormattingProvider = true;
#endif

      QueueManager::WriteStdout(kMethodType, out);

      // Set project root.
      EnsureEndsInSlash(project_path);
      config->projectRoot = project_path;
      // Create two cache directories for files inside and outside of the
      // project.
      MakeDirectoryRecursive(config->cacheDirectory +
                             EscapeFileName(config->projectRoot));
      MakeDirectoryRecursive(config->cacheDirectory + '@' +
                             EscapeFileName(config->projectRoot));

      Timer time;
      diag_engine->Init(config);
      semantic_cache->Init(config);

      // Open up / load the project.
      project->Load(config, project_path);
      time.ResetAndPrint("[perf] Loaded compilation entries (" +
                         std::to_string(project->entries.size()) + " files)");

      // Start indexer threads. Start this after loading the project, as that
      // may take a long time. Indexer threads will emit status/progress
      // reports.
      if (config->index.threads == 0) {
        // If the user has not specified how many indexers to run, try to
        // guess an appropriate value. Default to 80% utilization.
        const float kDefaultTargetUtilization = 0.8f;
        config->index.threads = (int)(std::thread::hardware_concurrency() *
                                      kDefaultTargetUtilization);
        if (config->index.threads <= 0)
          config->index.threads = 1;
      }
      LOG_S(INFO) << "Starting " << config->index.threads << " indexers";
      for (int i = 0; i < config->index.threads; ++i) {
        WorkThread::StartThread("indexer" + std::to_string(i), [=]() {
          Indexer_Main(config, diag_engine, file_consumer_shared,
                       timestamp_manager, import_manager,
                       import_pipeline_status, project, working_files, waiter);
        });
      }

      // Start scanning include directories before dispatching project
      // files, because that takes a long time.
      include_complete->Rescan();

      time.Reset();
      project->Index(config, QueueManager::instance(), working_files,
                     request->id);
      // We need to support multiple concurrent index processes.
      time.ResetAndPrint("[perf] Dispatched initial index requests");
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_Initialize);
}  // namespace
