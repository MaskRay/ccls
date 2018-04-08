#include "filesystem.hh"

#include "utils.h"

#include <queue>
#include <utility>

static void GetFilesInFolderHelper(
    std::string folder,
    bool recursive,
    std::string output_prefix,
    const std::function<void(const std::string&)>& handler) {
  std::queue<std::pair<fs::path, fs::path>> q;
  q.emplace(fs::path(folder), fs::path(output_prefix));
  while (!q.empty()) {
    for (auto it = fs::directory_iterator(q.front().first); it != fs::directory_iterator(); ++it) {
      auto path = it->path();
      std::string filename = path.filename();
      if (filename[0] != '.' || filename == ".ccls") {
        fs::file_status status = it->symlink_status();
        if (fs::is_regular_file(status))
          handler(q.front().second / filename);
        else if (fs::is_directory(status) || fs::is_symlink(status)) {
          if (recursive) {
            std::string child_dir = q.front().second / filename;
            if (fs::is_directory(status))
              q.push(make_pair(path, child_dir));
          }
        }
      }
    }
    q.pop();
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
