#include "lsp_diagnostic.h"

#include "match.h"
#include "working_files.h"

class DiagnosticsEngine {
  std::unique_ptr<GroupMatch> match_;
  int64_t nextPublish_ = 0;
  int frequencyMs_;

 public:
  void Init(Config*);
  void Publish(WorkingFiles* working_files,
               std::string path,
               std::vector<lsDiagnostic> diagnostics);
};
