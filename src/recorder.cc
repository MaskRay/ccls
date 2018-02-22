#include "recorder.h"

#include <loguru.hpp>

#include <cassert>
#include <fstream>

namespace {
std::ofstream* g_file_in = nullptr;
std::ofstream* g_file_out = nullptr;
}  // namespace

void EnableRecording(std::string path) {
  if (path.empty())
    path = "cquery";

  // We can only call |EnableRecording| once.
  assert(!g_file_in && !g_file_out);

  // Open the files.
  g_file_in = new std::ofstream(
      path + ".in", std::ios::out | std::ios::trunc | std::ios::binary);
  g_file_out = new std::ofstream(
      path + ".out", std::ios::out | std::ios::trunc | std::ios::binary);

  // Make sure we can write to the files.
  bool bad = false;
  if (!g_file_in->good()) {
    LOG_S(ERROR) << "record: cannot write to " << path << ".in";
    bad = true;
  }
  if (!g_file_out->good()) {
    LOG_S(ERROR) << "record: cannot write to " << path << ".out";
    bad = true;
  }
  if (bad) {
    delete g_file_in;
    delete g_file_out;
    g_file_in = nullptr;
    g_file_out = nullptr;
  }
}

void RecordInput(std::string_view content) {
  if (!g_file_in)
    return;
  (*g_file_in) << "Content-Length: " << content.size() << "\r\n\r\n" << content;
  (*g_file_in).flush();
}

void RecordOutput(std::string_view content) {
  if (!g_file_out)
    return;
  (*g_file_out) << content;
  (*g_file_out).flush();
}