#pragma once

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <functional>
#include <string>

void getFilesInFolder(std::string folder, bool recursive,
                      bool add_folder_to_path,
                      const std::function<void(const std::string &)> &handler);
