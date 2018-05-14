#include "filesystem.hh"
using namespace llvm;

#include "utils.h"

#include <utility>

void GetFilesInFolder(std::string folder,
                      bool recursive,
                      bool dir_prefix,
                      const std::function<void(const std::string&)>& handler) {
  EnsureEndsInSlash(folder);
  std::error_code ec;
  if (recursive)
    for (sys::fs::recursive_directory_iterator I(folder, ec), E; I != E && !ec;
         I.increment(ec)) {
      std::string path = I->path(), filename = sys::path::filename(path);
      if (filename[0] != '.' || filename == ".ccls") {
        if (!dir_prefix)
          path = path.substr(folder.size());
        handler(path);
      }
    }
  else
    for (sys::fs::directory_iterator I(folder, ec), E; I != E && !ec;
         I.increment(ec)) {
      std::string path = I->path(), filename = sys::path::filename(path);
      if (filename[0] != '.' || filename == ".ccls") {
        if (!dir_prefix)
          path = path.substr(folder.size());
        handler(path);
      }
    }
}
