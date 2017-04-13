#include "utils.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include <tinydir.h>

// See http://stackoverflow.com/a/2072890
bool EndsWith(const std::string& value, const std::string& ending) {
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool StartsWith(const std::string& value, const std::string& start) {
  if (start.size() > value.size())
    return false;
  return std::equal(start.begin(), start.end(), value.begin());
}

// See http://stackoverflow.com/a/29752943
std::string ReplaceAll(const std::string& source, const std::string& from, const std::string& to) {
  std::string result;
  result.reserve(source.length());  // avoids a few memory allocations

  std::string::size_type last_pos = 0;
  std::string::size_type find_pos;

  while (std::string::npos != (find_pos = source.find(from, last_pos))) {
    result.append(source, last_pos, find_pos - last_pos);
    result += to;
    last_pos = find_pos + from.length();
  }

  // Care for the rest after last occurrence
  result += source.substr(last_pos);

  return result;
}

static std::vector<std::string> GetFilesInFolderHelper(std::string folder, bool recursive, std::string output_prefix) {
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
        if (recursive) {
          // Note that we must always ignore the '.' and '..' directories, otherwise
          // this will loop infinitely. The above check handles that for us.
          for (std::string nested_file : GetFilesInFolderHelper(file.path, true /*recursive*/, output_prefix + file.name + "/"))
            result.push_back(nested_file);
        }
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

std::vector<std::string> GetFilesInFolder(std::string folder, bool recursive, bool add_folder_to_path) {
  assert(folder.size() > 0);
  if (folder[folder.size() - 1] != '/')
    folder += '/';

  return GetFilesInFolderHelper(folder, recursive, add_folder_to_path ? folder : "");
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

std::unordered_map<std::string, std::string> ParseTestExpectation(std::string filename) {
  bool in_output = false;

#if false
  #include "bar.h"

  void foo();

  /*
  // if no name is given assume to be this file name
  // no output section means we don't check that index.
  OUTPUT: bar.cc
  {}

  OUTPUT: bar.h
  {}
  */
#endif

  std::unordered_map<std::string, std::string> result;

  std::string active_output_filename;
  std::string active_output_contents;

  for (std::string line : ReadLines(filename)) {
    if (line == "*/")
      break;

    if (StartsWith(line, "OUTPUT:")) {
      if (in_output) {
        result[active_output_filename] = active_output_contents;
      }

      if (line.size() > 7)
        active_output_filename = line.substr(8);
      else
        active_output_filename = filename;
      active_output_contents = "";

      in_output = true;
    }
    else if (in_output)
      active_output_contents += line + "\n";
  }

  if (in_output)
    result[active_output_filename] = active_output_contents;
  return result;
}


void Fail(const std::string& message) {
  std::cerr << "Fatal error: " << message << std::endl;
  std::exit(1);
}

void WriteToFile(const std::string& filename, const std::string& content) {
  std::ofstream file(filename);
  file << content;
}
