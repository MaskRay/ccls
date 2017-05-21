#include "utils.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <iostream>
#include <fstream>
#include <locale>
#include <sstream>
#include <unordered_map>

#include <tinydir.h>

namespace {

// See http://stackoverflow.com/a/217605
// Trim from start (in place)
void TrimStart(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(),
    std::not1(std::ptr_fun<int, int>(std::isspace))));
}
// Trim from end (in place)
void TrimEnd(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
    std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}
// Trim from both ends (in place)
void Trim(std::string& s) {
  TrimStart(s);
  TrimEnd(s);
}

}  // namespace

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

bool AnyStartsWith(const std::vector<std::string>& values, const std::string& start) {
  return std::any_of(std::begin(values), std::end(values), [&start](const std::string& value) {
    return StartsWith(value, start);
  });
}

bool EndsWithAny(const std::string& value, const std::vector<std::string>& endings) {
  return std::any_of(std::begin(endings), std::end(endings), [&value](const std::string& ending) {
    return EndsWith(value, ending);
  });
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

std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter) {
  // http://stackoverflow.com/a/13172514
  std::vector<std::string> strings;

  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while ((pos = str.find(delimiter, prev)) != std::string::npos) {
    strings.push_back(str.substr(prev, pos - prev));
    prev = pos + 1;
  }

  // To get the last substring (or only, if delimiter is not found)
  strings.push_back(str.substr(prev));

  return strings;
}

static void GetFilesInFolderHelper(
    std::string folder, bool recursive, std::string output_prefix, const std::function<void(const std::string&)>& handler) {
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
          GetFilesInFolderHelper(file.path, true /*recursive*/, output_prefix + file.name + "/", handler);
        }
      }
      else {
        handler(output_prefix + file.name);
      }
    }

    if (tinydir_next(&dir) == -1) {
      perror("Error getting next file");
      goto bail;
    }
  }

bail:
  tinydir_close(&dir);
}

std::vector<std::string> GetFilesInFolder(std::string folder, bool recursive, bool add_folder_to_path) {
  EnsureEndsInSlash(folder);
  std::vector<std::string> result;
  GetFilesInFolderHelper(folder, recursive, add_folder_to_path ? folder : "", [&result](const std::string& path) {
    result.push_back(path);
  });
  return result;
}


void GetFilesInFolder(std::string folder, bool recursive, bool add_folder_to_path, const std::function<void(const std::string&)>& handler) {
  EnsureEndsInSlash(folder);
  GetFilesInFolderHelper(folder, recursive, add_folder_to_path ? folder : "", handler);
}

void EnsureEndsInSlash(std::string& path) {
  if (path.empty() || path[path.size() - 1] != '/')
    path += '/';
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

optional<std::string> ReadContent(const std::string& filename) {
  std::ifstream cache;
  cache.open(filename);
  if (!cache.good())
    return nullopt;

  return std::string(
    std::istreambuf_iterator<char>(cache),
    std::istreambuf_iterator<char>());
}

std::vector<std::string> ReadLines(std::string filename) {
  std::vector<std::string> result;

  std::ifstream input(filename);
  for (std::string line; SafeGetline(input, line); )
    result.push_back(line);

  return result;
}

std::vector<std::string> ToLines(const std::string& content, bool trim_whitespace) {
  std::vector<std::string> result;

  std::istringstream lines(content);

  std::string line;
  while (getline(lines, line)) {
    if (trim_whitespace)
      Trim(line);
    result.push_back(line);
  }

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

void UpdateTestExpectation(const std::string& filename, const std::string& expectation, const std::string& actual) {
  // Read the entire file into a string.
  std::ifstream in(filename);
  std::string str;
  str.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  in.close();

  // Replace expectation
  auto it = str.find(expectation);
  assert(it != std::string::npos);
  str.replace(it, expectation.size(), actual);

  // Write it back out.
  std::ofstream of(filename, std::ios::out | std::ios::trunc);
  of.write(str.c_str(), str.size());
  of.close();
}

void Fail(const std::string& message) {
  std::cerr << "Fatal error: " << message << std::endl;
  std::exit(1);
}

void WriteToFile(const std::string& filename, const std::string& content) {
  std::ofstream file(filename);
  file << content;
}
