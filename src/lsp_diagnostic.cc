#include "lsp_diagnostic.h"

#include "queue_manager.h"
#include "working_files.h"

void EmitDiagnostics(WorkingFiles* working_files,
                     std::string path,
                     std::vector<lsDiagnostic> diagnostics) {
  // Emit diagnostics.
  Out_TextDocumentPublishDiagnostics out;
  out.params.uri = lsDocumentUri::FromPath(path);
  out.params.diagnostics = diagnostics;
  QueueManager::WriteStdout(IpcId::TextDocumentPublishDiagnostics, out);

  // Cache diagnostics so we can show fixits.
  working_files->DoActionOnFile(path, [&](WorkingFile* working_file) {
                                        if (working_file)
                                          working_file->diagnostics_ = diagnostics;
                                      });
}
