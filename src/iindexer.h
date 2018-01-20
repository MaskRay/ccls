#pragma once

#include <optional.h>

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

// TODO:
//  - rename indexer.h to clang_indexer.h and pull out non-clang specific code
//    like IndexFile
//  - rename this file to indexer.h

struct Config;
struct IndexFile;
struct FileContents;
struct FileConsumerSharedState;
struct PerformanceImportFile;

// Abstracts away the actual indexing process. Each IIndexer instance is
// per-thread and constructing an instance may be extremely expensive (ie,
// acquire a lock) and should be done as rarely as possible.
struct IIndexer {
  struct TestEntry {
    std::string path;
    int num_indexes = 0;

    TestEntry(const std::string& path, int num_indexes);
  };

  static std::unique_ptr<IIndexer> MakeClangIndexer();
  static std::unique_ptr<IIndexer> MakeTestIndexer(
      std::initializer_list<TestEntry> entries);

  virtual ~IIndexer() = default;
  virtual optional<std::vector<std::unique_ptr<IndexFile>>> Index(
      Config* config,
      FileConsumerSharedState* file_consumer_shared,
      std::string file,
      const std::vector<std::string>& args,
      const std::vector<FileContents>& file_contents,
      PerformanceImportFile* perf) = 0;
};
