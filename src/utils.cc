#include "utils.h"

#include "platform.h"

#include <doctest/doctest.h>
#include <tinydir.h>
#include <loguru/loguru.hpp>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <unordered_map>

#if !defined(__APPLE__)
#include <sparsepp/spp_memory.h>
#endif

// DEFAULT_RESOURCE_DIRECTORY is passed with quotes for non-MSVC compilers, ie,
// foo vs "foo".
#if defined(_MSC_VER)
#define _STRINGIFY(x) #x
#define ENSURE_STRING_MACRO_ARGUMENT(x) _STRINGIFY(x)
#else
#define ENSURE_STRING_MACRO_ARGUMENT(x) x
#endif

// See http://stackoverflow.com/a/217605
void TrimStartInPlace(std::string& s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))));
}
void TrimEndInPlace(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<int, int>(std::isspace)))
              .base(),
          s.end());
}
void TrimInPlace(std::string& s) {
  TrimStartInPlace(s);
  TrimEndInPlace(s);
}
std::string Trim(std::string s) {
  TrimInPlace(s);
  return s;
}

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

bool AnyStartsWith(const std::vector<std::string>& values,
                   const std::string& start) {
  return std::any_of(
      std::begin(values), std::end(values),
      [&start](const std::string& value) { return StartsWith(value, start); });
}

bool StartsWithAny(const std::string& value,
                   const std::vector<std::string>& startings) {
  return std::any_of(std::begin(startings), std::end(startings),
                     [&value](const std::string& starting) {
                       return StartsWith(value, starting);
                     });
}

bool EndsWithAny(const std::string& value,
                 const std::vector<std::string>& endings) {
  return std::any_of(
      std::begin(endings), std::end(endings),
      [&value](const std::string& ending) { return EndsWith(value, ending); });
}

// See http://stackoverflow.com/a/29752943
std::string ReplaceAll(const std::string& source,
                       const std::string& from,
                       const std::string& to) {
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

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delimiter) {
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

std::string LowerPathIfCaseInsensitive(const std::string& path) {
  std::string result = path;
#if defined(_WIN32)
  for (size_t i = 0; i < result.size(); ++i)
    result[i] = (char)tolower(result[i]);
#endif
  return result;
}

static void GetFilesInFolderHelper(
    std::string folder,
    bool recursive,
    std::string output_prefix,
    const std::function<void(const std::string&)>& handler) {
  tinydir_dir dir;
  if (tinydir_open(&dir, folder.c_str()) == -1) {
    LOG_S(WARNING) << "Unable to open directory " << folder;
    goto bail;
  }

  while (dir.has_next) {
    tinydir_file file;
    if (tinydir_readfile(&dir, &file) == -1) {
      LOG_S(WARNING) << "Unable to read file " << file.name
                     << " when reading directory " << folder;
      goto bail;
    }

    // Skip all dot files.
    //
    // The nested ifs are intentional, branching order is subtle here.
    //
    // Note that in the future if we do support dot directories/files, we must
    // always ignore the '.' and '..' directories otherwise this will loop
    // infinitely.
    if (file.name[0] != '.') {
      if (file.is_dir) {
        if (recursive) {
          std::string child_dir = output_prefix + file.name + "/";
          if (!IsSymLink(child_dir))
            GetFilesInFolderHelper(file.path, true /*recursive*/, child_dir,
                                   handler);
        }
      } else {
        handler(output_prefix + file.name);
      }
    }

    if (tinydir_next(&dir) == -1) {
      LOG_S(WARNING) << "Unable to fetch next file when reading directory "
                     << folder;
      goto bail;
    }
  }

bail:
  tinydir_close(&dir);
}

std::vector<std::string> GetFilesInFolder(std::string folder,
                                          bool recursive,
                                          bool add_folder_to_path) {
  EnsureEndsInSlash(folder);
  std::vector<std::string> result;
  GetFilesInFolderHelper(
      folder, recursive, add_folder_to_path ? folder : "",
      [&result](const std::string& path) { result.push_back(path); });
  return result;
}

void GetFilesInFolder(std::string folder,
                      bool recursive,
                      bool add_folder_to_path,
                      const std::function<void(const std::string&)>& handler) {
  EnsureEndsInSlash(folder);
  GetFilesInFolderHelper(folder, recursive, add_folder_to_path ? folder : "",
                         handler);
}

void EnsureEndsInSlash(std::string& path) {
  if (path.empty() || path[path.size() - 1] != '/')
    path += '/';
}

std::string EscapeFileName(std::string path) {
  if (path.size() && path.back() == '/')
    path.pop_back();
  std::replace(path.begin(), path.end(), '\\', '_');
  std::replace(path.begin(), path.end(), '/', '_');
  std::replace(path.begin(), path.end(), ':', '_');
  return path;
}

// http://stackoverflow.com/a/6089413
std::istream& SafeGetline(std::istream& is, std::string& t) {
  t.clear();

  // The characters in the stream are read one-by-one using a std::streambuf.
  // That is faster than reading them one-by-one using the std::istream. Code
  // that uses streambuf this way must be guarded by a sentry object. The sentry
  // object performs various tasks, such as thread synchronization and updating
  // the stream state.

  std::istream::sentry se(is, true);
  std::streambuf* sb = is.rdbuf();

  for (;;) {
    int c = sb->sbumpc();
    if (c == EOF) {
      // Also handle the case when the last line has no line ending
      if (t.empty())
        is.setstate(std::ios::eofbit);
      return is;
    }

    t += (char)c;

    if (c == '\n')
      return is;
  }
}

optional<std::string> ReadContent(const std::string& filename) {
  std::ifstream cache;
  cache.open(filename);

  try {
    return std::string(std::istreambuf_iterator<char>(cache),
                       std::istreambuf_iterator<char>());
  } catch (std::ios_base::failure&) {
    return nullopt;
  }
}

std::vector<std::string> ReadLinesWithEnding(std::string filename) {
  std::vector<std::string> result;

  std::ifstream input(filename);
  for (std::string line; SafeGetline(input, line);)
    result.push_back(line);

  return result;
}

std::vector<std::string> ToLines(const std::string& content,
                                 bool trim_whitespace) {
  std::vector<std::string> result;

  std::istringstream lines(content);

  std::string line;
  while (getline(lines, line)) {
    if (trim_whitespace)
      TrimInPlace(line);
    result.push_back(line);
  }

  return result;
}

std::string TextReplacer::Apply(const std::string& content) {
  std::string result = content;

  for (const Replacement& replacement : replacements) {
    while (true) {
      size_t idx = result.find(replacement.from);
      if (idx == std::string::npos)
        break;

      result.replace(result.begin() + idx,
                     result.begin() + idx + replacement.from.size(),
                     replacement.to);
    }
  }

  return result;
}

void ParseTestExpectation(
    const std::string& filename,
    const std::vector<std::string>& lines_with_endings,
    TextReplacer* replacer,
    std::vector<std::string>* flags,
    std::unordered_map<std::string, std::string>* output_sections) {
#if false
#include "bar.h"

  void foo();

  /*
  // DOCS for TEXT_REPLACE:
  //  Each line under TEXT_REPLACE is a replacement, ie, the two entries will be
  //  considered equivalent. This is useful for USRs which vary across files.

  // DOCS for EXTRA_FLAGS:
  //  Additional flags to pass to clang.

  // DOCS for OUTPUT:
  //  If no name is given assume to be this file name. If there is not an output
  //  section for a file it is not checked.

  TEXT_REPLACE:
  foo <===> bar
  one <===> two

  EXTRA_FLAGS:
  -std=c++14

  OUTPUT:
  {}

  OUTPUT: bar.cc
  {}

  OUTPUT: bar.h
  {}
  */
#endif

  // Scan for TEXT_REPLACE:
  {
    bool in_output = false;
    for (std::string line : lines_with_endings) {
      TrimInPlace(line);

      if (StartsWith(line, "TEXT_REPLACE:")) {
        assert(!in_output && "multiple TEXT_REPLACE sections");
        in_output = true;
        continue;
      }

      if (in_output && line.empty())
        break;

      if (in_output) {
        static const std::string kKey = " <===> ";
        size_t index = line.find(kKey);
        LOG_IF_S(FATAL, index == std::string::npos)
            << " No '" << kKey << "' in replacement string '" << line << "'"
            << ", index=" << index;

        TextReplacer::Replacement replacement;
        replacement.from = line.substr(0, index);
        replacement.to = line.substr(index + kKey.size());
        TrimInPlace(replacement.from);
        TrimInPlace(replacement.to);
        replacer->replacements.push_back(replacement);
      }
    }
  }

  // Scan for EXTRA_FLAGS:
  {
    bool in_output = false;
    for (std::string line : lines_with_endings) {
      TrimInPlace(line);

      if (StartsWith(line, "EXTRA_FLAGS:")) {
        assert(!in_output && "multiple EXTRA_FLAGS sections");
        in_output = true;
        continue;
      }

      if (in_output && line.empty())
        break;

      if (in_output)
        flags->push_back(line);
    }
  }

  // Scan for OUTPUT:
  {
    std::string active_output_filename;
    std::string active_output_contents;

    bool in_output = false;
    for (std::string line_with_ending : lines_with_endings) {
      if (StartsWith(line_with_ending, "*/"))
        break;

      if (StartsWith(line_with_ending, "OUTPUT:")) {
        // Terminate the previous output section if we found a new one.
        if (in_output) {
          (*output_sections)[active_output_filename] = active_output_contents;
        }

        // Try to tokenize OUTPUT: based one whitespace. If there is more than
        // one token assume it is a filename.
        std::vector<std::string> tokens = SplitString(line_with_ending, " ");
        if (tokens.size() > 1) {
          active_output_filename = tokens[1];
          TrimInPlace(active_output_filename);
        } else {
          active_output_filename = filename;
        }
        active_output_contents = "";

        in_output = true;
      } else if (in_output)
        active_output_contents += line_with_ending;
    }

    if (in_output)
      (*output_sections)[active_output_filename] = active_output_contents;
  }
}

void UpdateTestExpectation(const std::string& filename,
                           const std::string& expectation,
                           const std::string& actual) {
  // Read the entire file into a string.
  std::ifstream in(filename);
  std::string str;
  str.assign(std::istreambuf_iterator<char>(in),
             std::istreambuf_iterator<char>());
  in.close();

  // Replace expectation
  auto it = str.find(expectation);
  assert(it != std::string::npos);
  str.replace(it, expectation.size(), actual);

  // Write it back out.
  std::ofstream of(filename,
                   std::ios::out | std::ios::trunc | std::ios::binary);
  of.write(str.c_str(), str.size());
  of.close();
}

void Fail(const std::string& message) {
  LOG_S(FATAL) << "Fatal error: " << message;
  std::exit(1);
}

void WriteToFile(const std::string& filename, const std::string& content) {
  std::ofstream file(filename);
  file << content;
}

float GetProcessMemoryUsedInMb() {
#if defined(__APPLE__)
  return 0.f;
#else
  const float kBytesToMb = 1000000;
  uint64_t memory_after = spp::GetProcessMemoryUsed();
  return memory_after / kBytesToMb;
#endif
}

std::string FormatMicroseconds(long long microseconds) {
  long long milliseconds = microseconds / 1000;
  long long remaining = microseconds - milliseconds;

  // Only show two digits after the dot.
  while (remaining >= 100)
    remaining /= 10;

  return std::to_string(milliseconds) + "." + std::to_string(remaining) + "ms";
}

std::string GetDefaultResourceDirectory() {
  std::string result;

  std::string resource_directory =
      std::string(ENSURE_STRING_MACRO_ARGUMENT(DEFAULT_RESOURCE_DIRECTORY));
  // Remove double quoted resource dir if it was passed with quotes
  // by the build system.
  if (resource_directory.size() >= 2 && resource_directory[0] == '"' &&
      resource_directory[resource_directory.size() - 1] == '"') {
    resource_directory =
        resource_directory.substr(1, resource_directory.size() - 2);
  }
  if (resource_directory.find("..") != std::string::npos) {
    std::string executable_path = GetExecutablePath();
    size_t pos = executable_path.find_last_of('/');
    result = executable_path.substr(0, pos + 1);
    result += resource_directory;
  } else {
    result = resource_directory;
  }

  return NormalizePath(result);
}

std::string UpdateToRnNewlines(std::string output) {
  size_t idx = 0;
  while (true) {
    idx = output.find('\n', idx);

    // No more matches.
    if (idx == std::string::npos)
      break;

    // Skip an existing "\r\n" match.
    if (idx > 0 && output[idx - 1] == '\r') {
      ++idx;
      continue;
    }

    // Replace "\n" with "\r|n".
    output.replace(output.begin() + idx, output.begin() + idx + 1, "\r\n");
  }

  return output;
};

TEST_SUITE("ParseTestExpectation") {
  TEST_CASE("Parse TEXT_REPLACE") {
    // clang-format off
    std::vector<std::string> lines_with_endings = {
        "/*\n",
        "TEXT_REPLACE:\n",
        "  foo   <===> \tbar  \n",
        "01 <===> 2\n",
        "\n",
        "*/\n"};
    // clang-format on

    TextReplacer text_replacer;
    std::vector<std::string> flags;
    std::unordered_map<std::string, std::string> all_expected_output;
    ParseTestExpectation("foo.cc", lines_with_endings, &text_replacer, &flags,
                         &all_expected_output);

    REQUIRE(text_replacer.replacements.size() == 2);
    REQUIRE(text_replacer.replacements[0].from == "foo");
    REQUIRE(text_replacer.replacements[0].to == "bar");
    REQUIRE(text_replacer.replacements[1].from == "01");
    REQUIRE(text_replacer.replacements[1].to == "2");
  }

  TEST_CASE("Apply TEXT_REPLACE") {
    TextReplacer replacer;
    replacer.replacements.push_back(TextReplacer::Replacement{"foo", "bar"});
    replacer.replacements.push_back(TextReplacer::Replacement{"01", "2"});
    replacer.replacements.push_back(TextReplacer::Replacement{"3", "456"});

    // Equal-length.
    REQUIRE(replacer.Apply("foo") == "bar");
    REQUIRE(replacer.Apply("bar") == "bar");

    // Shorter replacement.
    REQUIRE(replacer.Apply("01") == "2");
    REQUIRE(replacer.Apply("2") == "2");

    // Longer replacement.
    REQUIRE(replacer.Apply("3") == "456");
    REQUIRE(replacer.Apply("456") == "456");

    // Content before-after replacement.
    REQUIRE(replacer.Apply("aaaa01bbbb") == "aaaa2bbbb");

    // Multiple replacements.
    REQUIRE(replacer.Apply("foofoobar0123") == "barbarbar22456");
  }
}

TEST_SUITE("Update \\n to \\r\\n") {
  TEST_CASE("all") {
    REQUIRE(UpdateToRnNewlines("\n") == "\r\n");
    REQUIRE(UpdateToRnNewlines("\n\n") == "\r\n\r\n");
    REQUIRE(UpdateToRnNewlines("\r\n\n") == "\r\n\r\n");
    REQUIRE(UpdateToRnNewlines("\n\r\n") == "\r\n\r\n");
    REQUIRE(UpdateToRnNewlines("\r\n\r\n") == "\r\n\r\n");
    REQUIRE(UpdateToRnNewlines("f1\nfo2\nfoo3") == "f1\r\nfo2\r\nfoo3");
    REQUIRE(UpdateToRnNewlines("f1\r\nfo2\r\nfoo3") == "f1\r\nfo2\r\nfoo3");
  }
}
