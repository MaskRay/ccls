#include "diagnostics_engine.h"

#include "queue_manager.h"

#include <chrono>

void DiagnosticsEngine::Init(Config* config) {
  frequencyMs_ = config->diagnostics.frequencyMs;
  match_ = std::make_unique<GroupMatch>(config->diagnostics.whitelist,
                                        config->diagnostics.blacklist);
}

void DiagnosticsEngine::Publish(WorkingFiles* working_files,
                                std::string path,
                                std::vector<lsDiagnostic> diagnostics) {
  // Cache diagnostics so we can show fixits.
  working_files->DoActionOnFile(path, [&](WorkingFile* working_file) {
    if (working_file)
      working_file->diagnostics_ = diagnostics;
  });

  int64_t now =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  if (frequencyMs_ >= 0 && (nextPublish_ <= now || diagnostics.empty()) &&
      match_->IsMatch(path)) {
    nextPublish_ = now + frequencyMs_;

    Out_TextDocumentPublishDiagnostics out;
    out.params.uri = lsDocumentUri::FromPath(path);
    out.params.diagnostics = diagnostics;
    QueueManager::WriteStdout(kMethodType_TextDocumentPublishDiagnostics, out);
  }
}
