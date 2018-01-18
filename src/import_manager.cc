#include "import_manager.h"

bool ImportManager::TryMarkDependencyImported(const std::string& path) {
  std::lock_guard<std::mutex> lock(depdency_mutex_);
  return depdency_imported_.insert(path).second;
}

bool ImportManager::StartQueryDbImport(const std::string& path) {
  return querydb_processing_.insert(path).second;
}

void ImportManager::DoneQueryDbImport(const std::string& path) {
  querydb_processing_.erase(path);
}
