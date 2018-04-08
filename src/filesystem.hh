#pragma once

#include <experimental/filesystem>
#include <functional>
#include <string>

namespace fs = std::experimental::filesystem;

void GetFilesInFolder(std::string folder,
                      bool recursive,
                      bool add_folder_to_path,
                      const std::function<void(const std::string&)>& handler);
