#include "include_complete.h"

#include "match.h"
#include "platform.h"
#include "project.h"
#include "standard_includes.h"
#include "timer.h"

#include <thread>

namespace {

struct CompletionCandidate {
  std::string absolute_path;
  lsCompletionItem completion_item;
};

std::string ElideLongPath(Config* config, const std::string& path) {
  if (config->includeCompletionMaximumPathLength <= 0)
    return path;

  if (path.size() <= config->includeCompletionMaximumPathLength)
    return path;

  size_t start = path.size() - config->includeCompletionMaximumPathLength;
  return ".." + path.substr(start + 2);
}

size_t TrimCommonPathPrefix(const std::string& result, const std::string& trimmer) {
  size_t i = 0;
  while (i < result.size() && i < trimmer.size()) {
    char a = result[i];
    char b = trimmer[i];
#if defined(_WIN32)
    a = (char)tolower(a);
    b = (char)tolower(b);
#endif
    if (a != b)
      break;
    ++i;
  }

  if (i == trimmer.size())
    return i;
  return 0;
}

// Returns true iff angle brackets should be used.
bool TrimPath(Project* project, const std::string& project_root, std::string* insert_path) {
  size_t start = 0;
  bool angle = false;

  size_t len = TrimCommonPathPrefix(*insert_path, project_root);
  if (len > start)
    start = len;

  for (auto& include_dir : project->quote_include_directories) {
    len = TrimCommonPathPrefix(*insert_path, include_dir);
    if (len > start)
      start = len;
  }

  for (auto& include_dir : project->angle_include_directories) {
    len = TrimCommonPathPrefix(*insert_path, include_dir);
    if (len > start) {
      start = len;
      angle = true;
    }
  }

  *insert_path = insert_path->substr(start);
  return angle;
}

lsCompletionItem BuildCompletionItem(Config* config, const std::string& path, bool use_angle_brackets, bool is_stl) {
  lsCompletionItem item;
  if (use_angle_brackets)
    item.label = "#include <" + ElideLongPath(config, path) + ">";
  else
    item.label = "#include \"" + ElideLongPath(config, path) + "\"";

  item.detail = path;

  // Replace the entire existing content.
  // NOTE: When submitting completion items, textEdit->range must be updated.
  item.textEdit = lsTextEdit();
  if (use_angle_brackets)
    item.textEdit->newText = "#include <" + path + ">";
  else
    item.textEdit->newText = "#include \"" + path + "\"";

  item.insertTextFormat = lsInsertTextFormat::PlainText;
  if (is_stl)
    item.kind = lsCompletionItemKind::Module;
  else
    item.kind = lsCompletionItemKind::File;

  return item;
}

}  // namespace

IncludeComplete::IncludeComplete(Config* config, Project* project)
    : is_scanning(false), config_(config), project_(project) {}

void IncludeComplete::Rescan() {
  if (is_scanning)
    return;

  completion_items.clear();
  absolute_path_to_completion_item.clear();

  if (!match_ && (!config_->includeCompletionWhitelist.empty() || !config_->includeCompletionBlacklist.empty()))
    match_ = MakeUnique<GroupMatch>(config_->includeCompletionWhitelist, config_->includeCompletionBlacklist);

  is_scanning = true;
  new std::thread([this]() {
    SetCurrentThreadName("include_scanner");
    Timer timer;

    InsertStlIncludes();
    InsertIncludesFromDirectory(config_->projectRoot, false /*use_angle_brackets*/);
    for (const std::string& dir : project_->quote_include_directories)
      InsertIncludesFromDirectory(dir, false /*use_angle_brackets*/);
    for (const std::string& dir : project_->angle_include_directories)
      InsertIncludesFromDirectory(dir, true /*use_angle_brackets*/);

    timer.ResetAndPrint("[perf] Scanning for includes");
    is_scanning = false;
  });
}

void IncludeComplete::AddFile(const std::string& absolute_path) {
  if (!EndsWithAny(absolute_path, config_->includeCompletionWhitelistLiteralEnding))
    return;
  if (match_ && !match_->IsMatch(absolute_path))
    return;

  std::string trimmed_path = absolute_path;
  bool use_angle_brackets = TrimPath(project_, config_->projectRoot, &trimmed_path);
  lsCompletionItem item = BuildCompletionItem(config_, trimmed_path, use_angle_brackets, false /*is_stl*/);

  if (is_scanning) {
    std::lock_guard<std::mutex> lock(completion_items_mutex);
    if (absolute_path_to_completion_item.insert(std::make_pair(absolute_path, completion_items.size())).second)
      completion_items.push_back(item);
  }
  else {
    if (absolute_path_to_completion_item.insert(std::make_pair(absolute_path, completion_items.size())).second)
      completion_items.push_back(item);
  }
}

void IncludeComplete::InsertIncludesFromDirectory(
    std::string directory,
    bool use_angle_brackets) {
  directory = NormalizePath(directory);
  EnsureEndsInSlash(directory);

  std::vector<CompletionCandidate> results;
  GetFilesInFolder(directory, true /*recursive*/, false /*add_folder_to_path*/, [&](const std::string& path) {
    if (!EndsWithAny(path, config_->includeCompletionWhitelistLiteralEnding))
      return;
    if (match_ && !match_->IsMatch(directory + path))
      return;

    CompletionCandidate candidate;
    candidate.absolute_path = directory + path;
    candidate.completion_item = BuildCompletionItem(config_, path, use_angle_brackets, false /*is_stl*/);
    results.push_back(candidate);
  });

  std::lock_guard<std::mutex> lock(completion_items_mutex);
  for (const CompletionCandidate& result : results) {
    if (absolute_path_to_completion_item.insert(std::make_pair(result.absolute_path, completion_items.size())).second)
      completion_items.push_back(result.completion_item);
  }
}

void IncludeComplete::InsertStlIncludes() {
  std::lock_guard<std::mutex> lock(completion_items_mutex);
  for (const char* stl_header : kStandardLibraryIncludes) {
    completion_items.push_back(BuildCompletionItem(config_, stl_header, true /*use_angle_brackets*/, true /*is_stl*/));
  }
}

optional<lsCompletionItem> IncludeComplete::FindCompletionItemForAbsolutePath(const std::string& absolute_path) {
  std::lock_guard<std::mutex> lock(completion_items_mutex);

  auto it = absolute_path_to_completion_item.find(absolute_path);
  if (it == absolute_path_to_completion_item.end())
    return nullopt;
  return completion_items[it->second];
}
