#include "entry_points.h"
#include "message_handler.h"
#include "platform.h"
#include "timer.h"

#include <loguru.hpp>

struct InitializeHandler : BaseMessageHandler<Ipc_InitializeRequest> {
  void Run(Ipc_InitializeRequest* request) override {
    // Log initialization parameters.
    rapidjson::StringBuffer output;
    Writer writer(output);
    Reflect(writer, request->params.initializationOptions);
    LOG_S(INFO) << "Init parameters: " << output.GetString();

    if (request->params.rootUri) {
      std::string project_path = request->params.rootUri->GetPath();
      LOG_S(INFO) << "[querydb] Initialize in directory " << project_path
                  << " with uri " << request->params.rootUri->raw_uri;

      if (!request->params.initializationOptions) {
        LOG_S(FATAL) << "Initialization parameters (particularily "
                        "cacheDirectory) are required";
        exit(1);
      }

      *config = *request->params.initializationOptions;

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

      // Make sure cache directory is valid.
      if (config->cacheDirectory.empty()) {
        LOG_S(FATAL) << "Exiting; no cache directory";
        exit(1);
      }

      config->cacheDirectory = NormalizePath(config->cacheDirectory);
      EnsureEndsInSlash(config->cacheDirectory);

      // Ensure there is a resource directory.
      if (config->resourceDirectory.empty()) {
        config->resourceDirectory = GetWorkingDirectory();
#if defined(_WIN32)
        config->resourceDirectory += std::string("../../clang_resource_dir/");
#else
        config->resourceDirectory += std::string("../clang_resource_dir/");
#endif
      }
      config->resourceDirectory = NormalizePath(config->resourceDirectory);
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
      out.result.capabilities.textDocumentSync =
          lsTextDocumentSyncKind::Incremental;

      out.result.capabilities.renameProvider = true;

      out.result.capabilities.completionProvider = lsCompletionOptions();
      out.result.capabilities.completionProvider->resolveProvider = false;
      // vscode doesn't support trigger character sequences, so we use ':'
      // for
      // '::' and '>' for '->'. See
      // https://github.com/Microsoft/language-server-protocol/issues/138.
      out.result.capabilities.completionProvider->triggerCharacters = {
          ".", ":", ">", "#"};

      out.result.capabilities.signatureHelpProvider = lsSignatureHelpOptions();
      // NOTE: If updating signature help tokens make sure to also update
      // WorkingFile::FindClosestCallNameInBuffer.
      out.result.capabilities.signatureHelpProvider->triggerCharacters = {"(",
                                                                          ","};

      out.result.capabilities.codeLensProvider = lsCodeLensOptions();
      out.result.capabilities.codeLensProvider->resolveProvider = false;

      out.result.capabilities.definitionProvider = true;
      out.result.capabilities.documentHighlightProvider = true;
      out.result.capabilities.hoverProvider = true;
      out.result.capabilities.referencesProvider = true;

      out.result.capabilities.codeActionProvider = true;

      out.result.capabilities.documentSymbolProvider = true;
      out.result.capabilities.workspaceSymbolProvider = true;

      out.result.capabilities.documentLinkProvider = lsDocumentLinkOptions();
      out.result.capabilities.documentLinkProvider->resolveProvider = false;

      IpcManager::WriteStdout(IpcId::Initialize, out);

      // Set project root.
      config->projectRoot = NormalizePath(request->params.rootUri->GetPath());
      EnsureEndsInSlash(config->projectRoot);
      MakeDirectoryRecursive(config->cacheDirectory +
                             EscapeFileName(config->projectRoot));

      // Start indexer threads.
      if (config->indexerCount == 0) {
        // If the user has not specified how many indexers to run, try to
        // guess an appropriate value. Default to 80% utilization.
        const float kDefaultTargetUtilization = 0.8;
        config->indexerCount =
            std::thread::hardware_concurrency() * kDefaultTargetUtilization;
        if (config->indexerCount <= 0)
          config->indexerCount = 1;
      }
      LOG_S(INFO) << "Starting " << config->indexerCount << " indexers";
      for (int i = 0; i < config->indexerCount; ++i) {
        WorkThread::StartThread("indexer" + std::to_string(i), [=]() {
          return IndexMain(config, file_consumer_shared, timestamp_manager,
                           import_manager, project, working_files, waiter,
                           queue);
        });
      }

      Timer time;

      // Open up / load the project.
      project->Load(config->extraClangArguments,
                    config->compilationDatabaseDirectory, project_path,
                    config->resourceDirectory);
      time.ResetAndPrint("[perf] Loaded compilation entries (" +
                         std::to_string(project->entries.size()) + " files)");

      // Start scanning include directories before dispatching project
      // files, because that takes a long time.
      include_complete->Rescan();

      time.Reset();
      project->ForAllFilteredFiles(
          config, [&](int i, const Project::Entry& entry) {
            bool is_interactive =
                working_files->GetFileByFilename(entry.filename) != nullptr;
            queue->index_request.Enqueue(Index_Request(
                entry.filename, entry.args, is_interactive, nullopt));
          });

      // We need to support multiple concurrent index processes.
      time.ResetAndPrint("[perf] Dispatched initial index requests");
    }
  }
};
REGISTER_MESSAGE_HANDLER(InitializeHandler);
