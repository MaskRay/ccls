#pragma once

#include "serializer.h"

#include <cstdint>

// Contains timing information for the entire pipeline for importing a file
// into the querydb.
struct PerformanceImportFile {
  // All units are in microseconds.

  // [indexer] clang parsing the file
  uint64_t index_parse = 0;
  // [indexer] build the IndexFile object from clang parse
  uint64_t index_build = 0;
  // [indexer] save the IndexFile to disk
  uint64_t index_save_to_disk = 0;
  // [indexer] loading previously cached index
  uint64_t index_load_cached = 0;
  // [indexer] create delta IndexUpdate object
  uint64_t index_make_delta = 0;
  // [querydb] update WorkingFile indexed file state
  // uint64_t querydb_update_working_file = 0;
  // [querydb] apply IndexUpdate
  // uint64_t querydb_apply_index_update = 0;
};
MAKE_REFLECT_STRUCT(PerformanceImportFile,
                    index_parse,
                    index_build,
                    index_save_to_disk,
                    index_load_cached,
                    index_make_delta);
