#pragma once

// Contains timing information for the entire pipeline for importing a file
// into the querydb.
struct PerformanceImportFile {
  // All units are in microseconds.

  // [indexer] clang parsing the file
  long long index_parse = 0;
  // [indexer] build the IndexFile object from clang parse
  long long index_build = 0;
  // [querydb] create IdMap object from IndexFile
  long long querydb_id_map = 0;
  // [indexer] save the IndexFile to disk
  long long index_save_to_disk = 0;
  // [indexer] loading previously cached index
  long long index_load_cached = 0;
  // [indexer] create delta IndexUpdate object
  long long index_make_delta = 0;
  // [querydb] update WorkingFile indexed file state
  //long long querydb_update_working_file = 0;
  // [querydb] apply IndexUpdate
  //long long querydb_apply_index_update = 0;
};