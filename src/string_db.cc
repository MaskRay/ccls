#if false
#include "string_db.h"

StringStorage::~StringStorage() {
  if (owns_str) {
    free((void*)str);
    str = nullptr;
    owns_str= false;
  }
}

StringStorage StringStorage::CreateUnowned(const char* data, size_t len) {
  StringStorage result;
  result.str = data;
  result.len = len;
  result.owns_str = false;
  return result;
}

void StringStorage::Copy() const {
  // Copy
  char* new_str = (char*)malloc(len + 1);
  strncpy(new_str, str, len + 1);
  // Assign
  str = new_str;
  owns_str = true;
}

bool StringStorage::operator==(const StringStorage& that) const {
  return len == that.len && strcmp(str, that.str) == 0;
}

bool StringStorage::operator!=(const StringStorage& that) const {
  return !(*this == that);
}

bool StringStorage::operator<(const StringStorage& that) const {
  return strcmp(str, that.str);
}
#endif