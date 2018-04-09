#include "iindexer.h"

#include "indexer.h"

namespace {
struct ClangIndexer : IIndexer {
  ~ClangIndexer() override = default;

  std::vector<std::unique_ptr<IndexFile>> Index(
      FileConsumerSharedState* file_consumer_shared,
      std::string file,
      const std::vector<std::string>& args,
      const std::vector<FileContents>& file_contents,
      PerformanceImportFile* perf) override {
    return Parse(file_consumer_shared, file, args, file_contents, perf, &index);
  }

  // Note: constructing this acquires a global lock
  ClangIndex index;
};

struct TestIndexer : IIndexer {
  static std::unique_ptr<TestIndexer> FromEntries(
      const std::vector<TestEntry>& entries) {
    auto result = std::make_unique<TestIndexer>();

    for (const TestEntry& entry : entries) {
      std::vector<std::unique_ptr<IndexFile>> indexes;

      if (entry.num_indexes > 0)
        indexes.push_back(std::make_unique<IndexFile>(entry.path, "<empty>"));
      for (int i = 1; i < entry.num_indexes; ++i) {
        indexes.push_back(std::make_unique<IndexFile>(
            entry.path + "_extra_" + std::to_string(i) + ".h", "<empty>"));
      }

      result->indexes.insert(std::make_pair(entry.path, std::move(indexes)));
    }

    return result;
  }

  ~TestIndexer() override = default;

  std::vector<std::unique_ptr<IndexFile>> Index(
      FileConsumerSharedState* file_consumer_shared,
      std::string file,
      const std::vector<std::string>& args,
      const std::vector<FileContents>& file_contents,
      PerformanceImportFile* perf) override {
    auto it = indexes.find(file);
    if (it == indexes.end()) {
      // Don't return any indexes for unexpected data.
      assert(false && "no indexes");
      return {};
    }

    // FIXME: allow user to control how many times we return the index for a
    // specific file (atm it is always 1)
    auto result = std::move(it->second);
    indexes.erase(it);
    return result;
  }

  std::unordered_map<std::string, std::vector<std::unique_ptr<IndexFile>>>
      indexes;
};

}  // namespace

IIndexer::TestEntry::TestEntry(const std::string& path, int num_indexes)
    : path(path), num_indexes(num_indexes) {}

// static
std::unique_ptr<IIndexer> IIndexer::MakeClangIndexer() {
  return std::make_unique<ClangIndexer>();
}

// static
std::unique_ptr<IIndexer> IIndexer::MakeTestIndexer(
    std::initializer_list<TestEntry> entries) {
  return TestIndexer::FromEntries(entries);
}
