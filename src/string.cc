#include "buffer.h"

#include <cstring>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

struct CXString {}; // TODO

struct StringView {
  StringView(const char* str, size_t len);

  const char* const str;
  size_t len;
};

struct StringDb {
  StringView GetString(const std::string& str);
  StringView GetString(const char* str);
  StringView GetString(CXString cx_string);

  struct StringStorage {
    ~StringStorage();

    static StringStorage CreateUnowned(const char* data, size_t len);
    void Copy();

    const char* data;
    size_t len;
    bool owns_data;
  };

  std::unordered_set<StringStorage> data_;
};

StringDb::StringStorage::~StringStorage() {
  if (owns_data) {
    free((void*)data);
    data = nullptr;
    owns_data = false;
  }
}

StringDb::StringStorage StringDb::StringStorage::CreateUnowned(const char* data,
                                                               size_t len) {
  StringStorage result;
  result.data = data;
  result.len = len;
  result.owns_data = false;
  return result;
}

void StringDb::StringStorage::Copy() {
  // Copy
  char* new_data = (char*)malloc(len + 1);
  strncpy(new_data, data, len + 1);
  // Assign
  data = new_data;
  owns_data = true;
}

StringView::StringView(const char* str, size_t len) : str(str), len(len) {}

StringView StringDb::GetString(const std::string& str) {
  StringStorage lookup = StringStorage::CreateUnowned(str.c_str(), str.length());

  auto it = data_.insert(str);
  if (it.second)
    it.first->Copy();

  return StringView(it.first->c_str(), it.first->length());
}

StringView StringDb::GetString(CXString cx_string) {
  assert(cx_string.data);

  StringView result string=clang_getCString(cx_string);
  clang_disposeString(cx_string);

}


