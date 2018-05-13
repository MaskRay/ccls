#include "filesystem.hh"
using namespace llvm;

#include "utils.h"

#include <utility>

static void GetFilesInFolderHelper(
    std::string folder,
    bool recursive,
    std::string output_prefix,
    const std::function<void(const std::string&)>& handler) {
  std::error_code ec;
  if (recursive)
    for (sys::fs::recursive_directory_iterator I(folder, ec), E; I != E && !ec;
         I.increment(ec)) {
      std::string path = I->path(), filename = sys::path::filename(path);
      if (filename[0] != '.' || filename == ".ccls") {
        SmallString<256> Path;
        if (output_prefix.size()) {
          sys::path::append(Path, output_prefix, path);
          handler(Path.str());
        } else
          handler(path);
      }
    }
  else
    for (sys::fs::directory_iterator I(folder, ec), E; I != E && !ec;
         I.increment(ec)) {
      std::string path = I->path(), filename = sys::path::filename(path);
      if (filename[0] != '.' || filename == ".ccls") {
        SmallString<256> Path;
        if (output_prefix.size()) {
          sys::path::append(Path, output_prefix, path);
          handler(Path.str());
        } else
          handler(path);
      }
    }
}

void GetFilesInFolder(std::string folder,
                      bool recursive,
                      bool add_folder_to_path,
                      const std::function<void(const std::string&)>& handler) {
  EnsureEndsInSlash(folder);
  GetFilesInFolderHelper(folder, recursive, add_folder_to_path ? folder : "",
                         handler);
}
