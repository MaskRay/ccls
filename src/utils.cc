// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "utils.hh"

#include "log.hh"
#include "message_handler.hh"
#include "pipeline.hh"
#include "platform.hh"

#include <siphash.h>

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <functional>
#include <regex>
#include <string.h>
#include <unordered_map>

using namespace llvm;

namespace ccls {
struct Matcher::Impl {
  std::regex regex;
};

Matcher::Matcher(const std::string &pattern)
    : impl(std::make_unique<Impl>()), pattern(pattern) {
  impl->regex = std::regex(pattern, std::regex_constants::ECMAScript |
                                        std::regex_constants::icase |
                                        std::regex_constants::optimize);
}

Matcher::~Matcher() {}

bool Matcher::matches(const std::string &text) const {
  return std::regex_search(text, impl->regex, std::regex_constants::match_any);
}

GroupMatch::GroupMatch(const std::vector<std::string> &whitelist,
                       const std::vector<std::string> &blacklist) {
  auto err = [](const std::string &pattern, const char *what) {
    ShowMessageParam params;
    params.type = MessageType::Error;
    params.message =
        "failed to parse EMCAScript regex " + pattern + " : " + what;
    pipeline::notify(window_showMessage, params);
  };
  for (const std::string &pattern : whitelist) {
    try {
      this->whitelist.emplace_back(pattern);
    } catch (const std::exception &e) {
      err(pattern, e.what());
    }
  }
  for (const std::string &pattern : blacklist) {
    try {
      this->blacklist.emplace_back(pattern);
    } catch (const std::exception &e) {
      err(pattern, e.what());
    }
  }
}

bool GroupMatch::matches(const std::string &text,
                         std::string *blacklist_pattern) const {
  for (const Matcher &m : whitelist)
    if (m.matches(text))
      return true;
  for (const Matcher &m : blacklist)
    if (m.matches(text)) {
      if (blacklist_pattern)
        *blacklist_pattern = m.pattern;
      return false;
    }
  return true;
}

uint64_t hashUsr(llvm::StringRef s) {
  union {
    uint64_t ret;
    uint8_t out[8];
  };
  // k is an arbitrary key. Don't change it.
  const uint8_t k[16] = {0xd0, 0xe5, 0x4d, 0x61, 0x74, 0x63, 0x68, 0x52,
                         0x61, 0x79, 0xea, 0x70, 0xca, 0x70, 0xf0, 0x0d};
  (void)siphash(reinterpret_cast<const uint8_t *>(s.data()), s.size(), k, out,
                8);
  return ret;
}

std::string lowerPathIfInsensitive(const std::string &path) {
#if defined(_WIN32)
  std::string ret = path;
  for (char &c : ret)
    c = tolower(c);
  return ret;
#else
  return path;
#endif
}

void ensureEndsInSlash(std::string &path) {
  if (path.empty() || path[path.size() - 1] != '/')
    path += '/';
}

std::string escapeFileName(std::string path) {
  bool slash = path.size() && path.back() == '/';
#ifdef _WIN32
  std::replace(path.begin(), path.end(), ':', '@');
#endif
  std::replace(path.begin(), path.end(), '/', '@');
  if (slash)
    path += '/';
  return path;
}

std::string resolveIfRelative(const std::string &directory,
                              const std::string &path) {
  if (sys::path::is_absolute(path))
    return path;
  SmallString<256> ret;
  sys::path::append(ret, directory, path);
  return normalizePath(ret.str());
}

std::string realPath(const std::string &path) {
  SmallString<256> buf;
  sys::fs::real_path(path, buf);
  return buf.empty() ? path : llvm::sys::path::convert_to_slash(buf);
}

bool normalizeFolder(std::string &path) {
  for (auto &[root, real] : g_config->workspaceFolders)
    if (real.size() && llvm::StringRef(path).startswith(real)) {
      path = root + path.substr(real.size());
      return true;
    }
  return false;
}

std::optional<int64_t> lastWriteTime(const std::string &path) {
  sys::fs::file_status status;
  if (sys::fs::status(path, status))
    return {};
  return sys::toTimeT(status.getLastModificationTime());
}

std::optional<std::string> readContent(const std::string &filename) {
  char buf[4096];
  std::string ret;
  FILE *f = fopen(filename.c_str(), "rb");
  if (!f)
    return {};
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0)
    ret.append(buf, n);
  fclose(f);
  return ret;
}

void writeToFile(const std::string &filename, const std::string &content) {
  FILE *f = fopen(filename.c_str(), "wb");
  if (!f ||
      (content.size() && fwrite(content.c_str(), content.size(), 1, f) != 1)) {
    LOG_S(ERROR) << "failed to write to " << filename << ' ' << strerror(errno);
    return;
  }
  fclose(f);
}

// Find discontinous |search| in |content|.
// Return |found| and the count of skipped chars before found.
int reverseSubseqMatch(std::string_view pat, std::string_view text,
                       int case_sensitivity) {
  if (case_sensitivity == 1)
    case_sensitivity = std::any_of(pat.begin(), pat.end(), isupper) ? 2 : 0;
  int j = pat.size();
  if (!j)
    return text.size();
  for (int i = text.size(); i--;)
    if ((case_sensitivity ? text[i] == pat[j - 1]
                          : tolower(text[i]) == tolower(pat[j - 1])) &&
        !--j)
      return i;
  return -1;
}

std::string getDefaultResourceDirectory() { return CLANG_RESOURCE_DIRECTORY; }
} // namespace ccls
