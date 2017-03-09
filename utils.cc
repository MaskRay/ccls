#include "utils.h"

#include <cassert>
#include <iostream>
#include <fstream>

#include "tinydir.h"

static std::vector<std::string> GetFilesInFolderHelper(std::string folder, std::string output_prefix) {
  std::vector<std::string> result;

  tinydir_dir dir;
  if (tinydir_open(&dir, folder.c_str()) == -1) {
    perror("Error opening file");
    goto bail;
  }

  while (dir.has_next) {
    tinydir_file file;
    if (tinydir_readfile(&dir, &file) == -1) {
      perror("Error getting file");
      goto bail;
    }

    // Skip all dot files.
    if (file.name[0] != '.') {
      if (file.is_dir) {
        // Note that we must always ignore the '.' and '..' directories, otherwise
        // this will loop infinitely. The above check handles that for us.
        for (std::string nested_file : GetFilesInFolderHelper(file.path, output_prefix + file.name + "/"))
          result.push_back(nested_file);
      }
      else {
        result.push_back(output_prefix + file.name);
      }
    }

    if (tinydir_next(&dir) == -1) {
      perror("Error getting next file");
      goto bail;
    }
  }

bail:
  tinydir_close(&dir);
  return result;
}

std::vector<std::string> GetFilesInFolder(std::string folder, bool add_folder_to_path) {
  assert(folder.size() > 0);
  if (folder[folder.size() - 1] != '/')
    folder += '/';

  return GetFilesInFolderHelper(folder, add_folder_to_path ? folder : "");
}

// http://stackoverflow.com/a/6089413
std::istream& SafeGetline(std::istream& is, std::string& t) {
    t.clear();

    // The characters in the stream are read one-by-one using a std::streambuf.
    // That is faster than reading them one-by-one using the std::istream.
    // Code that uses streambuf this way must be guarded by a sentry object.
    // The sentry object performs various tasks,
    // such as thread synchronization and updating the stream state.

    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
            if(sb->sgetc() == '\n')
                sb->sbumpc();
            return is;
        case EOF:
            // Also handle the case when the last line has no line ending
            if(t.empty())
                is.setstate(std::ios::eofbit);
            return is;
        default:
            t += (char)c;
        }
    }
}

std::vector<std::string> ReadLines(std::string filename) {
  std::vector<std::string> result;

  std::ifstream input(filename);
  for (std::string line; SafeGetline(input, line); )
    result.push_back(line);

  return result;
}

void ParseTestExpectation(std::string filename, std::string* expected_output) {
  bool in_output = false;

  for (std::string line : ReadLines(filename)) {
    if (line == "*/")
      break;

    if (in_output)
      *expected_output += line + "\n";

    if (line == "OUTPUT:")
      in_output = true;
  }
}


void Fail(const std::string& message) {
  std::cerr << "Fatal error: " << message << std::endl;
  std::exit(1);
}

void WriteToFile(const std::string& filename, const std::string& content) {
  std::ofstream file(filename);
  file << content;
}
