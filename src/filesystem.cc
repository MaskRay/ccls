#include "filesystem.hh"
using namespace llvm;

#include "utils.h"

#include <vector>

void GetFilesInFolder(std::string folder,
                      bool recursive,
                      bool dir_prefix,
                      const std::function<void(const std::string&)>& handler) {
  EnsureEndsInSlash(folder);
  std::vector<std::string> st{folder};
  while (st.size()) {
    std::error_code ec;
    folder = st.back();
    st.pop_back();
    for (sys::fs::directory_iterator I(folder, ec), E; I != E && !ec;
         I.increment(ec)) {
      auto Status = I->status();
      if (!Status) continue;
      std::string path = I->path(), filename = sys::path::filename(path);
      if (filename[0] != '.' || filename == ".ccls") {
        if (sys::fs::is_regular_file(*Status)) {
          if (!dir_prefix)
            path = path.substr(folder.size());
          handler(path);
        } else if (recursive && sys::fs::is_directory(*Status))
          st.push_back(path);
      }
    }
  }
}
