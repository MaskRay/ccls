#include "lsp_diagnostic.h"

#include "match.h"
#include "working_files.h"

class DiagnosticsEngine {
  int frequencyMs_ = 0;
  GroupMatch match_;
  int64_t nextPublish_ = 0;

 public:
  DiagnosticsEngine(Config* config);
  void SetFrequencyMs(int ms) { frequencyMs_ = ms; }
  void Publish(WorkingFiles* working_files,
               std::string path,
               std::vector<lsDiagnostic> diagnostics);
};
