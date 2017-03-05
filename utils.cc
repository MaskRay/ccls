#include "utils.h"

#include <iostream>
#include <fstream>

#include "tinydir.h"

std::vector<std::string> GetFilesInFolder(std::string folder, bool add_folder_to_path) {
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

    std::string full_path;
    if (add_folder_to_path)
      full_path = folder + "/";
    full_path += file.name;
    if (file.is_dir) {
      // Ignore all dot directories.
      // Note that we must always ignore the '.' and '..' directories, otherwise
      // this will loop infinitely.
      if (file.name[0] != '.') {
        for (std::string nested_file : GetFilesInFolder(full_path, true /*add_folder_to_path*/))
          result.push_back(nested_file);
      }
    }
    else {
      result.push_back(full_path);
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

std::vector<std::string> ReadLines(std::string filename) {
  std::vector<std::string> result;

  std::ifstream input(filename);
  for (std::string line; getline(input, line); )
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