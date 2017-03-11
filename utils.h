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
