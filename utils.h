#pragma once

#include <string>
#include <vector>
#include <memory>

// Finds all files in the given folder. This is recursive.
std::vector<std::string> GetFilesInFolder(std::string folder, bool recursive, bool add_folder_to_path);
std::vector<std::string> ReadLines(std::string filename);
void ParseTestExpectation(std::string filename, std::string* expected_output);

void Fail(const std::string& message);


void WriteToFile(const std::string& filename, const std::string& content);

// note: this implementation does not disable this overload for array types
// See http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique#Possible_Implementatiog
template<typename T, typename... Args>
std::unique_ptr<T> MakeUnique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}


// http://stackoverflow.com/a/38140932
//
//  struct SomeHashKey {
//    std::string key1;
//    std::string key2;
//    bool key3;
//  };
//  MAKE_HASHABLE(SomeHashKey, t.key1, t.key2, t.key3)

inline void hash_combine(std::size_t& seed) { }

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  hash_combine(seed, rest...);
}

#define MAKE_HASHABLE(type, ...) \
    namespace std {\
        template<> struct hash<type> {\
            std::size_t operator()(const type &t) const {\
                std::size_t ret = 0;\
                hash_combine(ret, __VA_ARGS__);\
                return ret;\
            }\
        };\
    }