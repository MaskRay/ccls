#include "lsp.h"

#include "log.hh"
#include "serializers/json.h"

#include <rapidjson/writer.h>

#include <stdio.h>

MessageRegistry *MessageRegistry::instance_ = nullptr;

lsTextDocumentIdentifier
lsVersionedTextDocumentIdentifier::AsTextDocumentIdentifier() const {
  lsTextDocumentIdentifier result;
  result.uri = uri;
  return result;
}

// Reads a JsonRpc message. |read| returns the next input character.
std::optional<std::string>
ReadJsonRpcContentFrom(std::function<std::optional<char>()> read) {
  // Read the content length. It is terminated by the "\r\n" sequence.
  int exit_seq = 0;
  std::string stringified_content_length;
  while (true) {
    std::optional<char> opt_c = read();
    if (!opt_c) {
      LOG_S(INFO) << "No more input when reading content length header";
      return std::nullopt;
    }
    char c = *opt_c;

    if (exit_seq == 0 && c == '\r')
      ++exit_seq;
    if (exit_seq == 1 && c == '\n')
      break;

    stringified_content_length += c;
  }
  const char *kContentLengthStart = "Content-Length: ";
  assert(StartsWith(stringified_content_length, kContentLengthStart));
  int content_length =
      atoi(stringified_content_length.c_str() + strlen(kContentLengthStart));

  // There is always a "\r\n" sequence before the actual content.
  auto expect_char = [&](char expected) {
    std::optional<char> opt_c = read();
    return opt_c && *opt_c == expected;
  };
  if (!expect_char('\r') || !expect_char('\n')) {
    LOG_S(INFO) << "Unexpected token (expected \\r\\n sequence)";
    return std::nullopt;
  }

  // Read content.
  std::string content;
  content.reserve(content_length);
  for (int i = 0; i < content_length; ++i) {
    std::optional<char> c = read();
    if (!c) {
      LOG_S(INFO) << "No more input when reading content body";
      return std::nullopt;
    }
    content += *c;
  }

  return content;
}

std::optional<char> ReadCharFromStdinBlocking() {
  // We do not use std::cin because it does not read bytes once stuck in
  // cin.bad(). We can call cin.clear() but C++ iostream has other annoyance
  // like std::{cin,cout} is tied by default, which causes undesired cout flush
  // for cin operations.
  int c = getchar();
  if (c >= 0)
    return c;
  return std::nullopt;
}

std::optional<std::string>
MessageRegistry::ReadMessageFromStdin(std::unique_ptr<InMessage> *message) {
  std::optional<std::string> content =
      ReadJsonRpcContentFrom(&ReadCharFromStdinBlocking);
  if (!content) {
    LOG_S(ERROR) << "Failed to read JsonRpc input; exiting";
    exit(1);
  }

  rapidjson::Document document;
  document.Parse(content->c_str(), content->length());
  assert(!document.HasParseError());

  JsonReader json_reader{&document};
  return Parse(json_reader, message);
}

std::optional<std::string>
MessageRegistry::Parse(Reader &visitor, std::unique_ptr<InMessage> *message) {
  if (!visitor.HasMember("jsonrpc") ||
      std::string(visitor["jsonrpc"]->GetString()) != "2.0") {
    LOG_S(FATAL) << "Bad or missing jsonrpc version";
    exit(1);
  }

  std::string method;
  ReflectMember(visitor, "method", method);

  if (allocators.find(method) == allocators.end())
    return std::string("Unable to find registered handler for method '") +
           method + "'";

  Allocator &allocator = allocators[method];
  try {
    allocator(visitor, message);
    return std::nullopt;
  } catch (std::invalid_argument &e) {
    // *message is partially deserialized but some field (e.g. |id|) are likely
    // available.
    return std::string("Fail to parse '") + method + "' " +
           static_cast<JsonReader &>(visitor).GetPath() + ", expected " +
           e.what();
  }
}

MessageRegistry *MessageRegistry::instance() {
  if (!instance_)
    instance_ = new MessageRegistry();

  return instance_;
}

lsBaseOutMessage::~lsBaseOutMessage() = default;

void lsBaseOutMessage::Write(std::ostream &out) {
  rapidjson::StringBuffer output;
  rapidjson::Writer<rapidjson::StringBuffer> writer(output);
  JsonWriter json_writer{&writer};
  ReflectWriter(json_writer);

  out << "Content-Length: " << output.GetSize() << "\r\n\r\n"
      << output.GetString();
  out.flush();
}

void lsResponseError::Write(Writer &visitor) {
  auto &value = *this;
  int code2 = static_cast<int>(this->code);

  visitor.StartObject();
  REFLECT_MEMBER2("code", code2);
  REFLECT_MEMBER(message);
  visitor.EndObject();
}

lsDocumentUri lsDocumentUri::FromPath(const std::string &path) {
  lsDocumentUri result;
  result.SetPath(path);
  return result;
}

bool lsDocumentUri::operator==(const lsDocumentUri &other) const {
  return raw_uri == other.raw_uri;
}

void lsDocumentUri::SetPath(const std::string &path) {
  // file:///c%3A/Users/jacob/Desktop/superindex/indexer/full_tests
  raw_uri = path;

  size_t index = raw_uri.find(":");
  if (index == 1) { // widows drive letters must always be 1 char
    raw_uri.replace(raw_uri.begin() + index, raw_uri.begin() + index + 1,
                    "%3A");
  }

  // subset of reserved characters from the URI standard
  // http://www.ecma-international.org/ecma-262/6.0/#sec-uri-syntax-and-semantics
  std::string t;
  t.reserve(8 + raw_uri.size());
  // TODO: proper fix
#if defined(_WIN32)
  t += "file:///";
#else
  t += "file://";
#endif

  // clang-format off
  for (char c : raw_uri)
    switch (c) {
    case ' ': t += "%20"; break;
    case '#': t += "%23"; break;
    case '$': t += "%24"; break;
    case '&': t += "%26"; break;
    case '(': t += "%28"; break;
    case ')': t += "%29"; break;
    case '+': t += "%2B"; break;
    case ',': t += "%2C"; break;
    case ';': t += "%3B"; break;
    case '?': t += "%3F"; break;
    case '@': t += "%40"; break;
    default: t += c; break;
    }
  // clang-format on
  raw_uri = std::move(t);
}

std::string lsDocumentUri::GetPath() const {
  if (raw_uri.compare(0, 8, "file:///"))
    return raw_uri;
  std::string ret;
#ifdef _WIN32
  size_t i = 8;
#else
  size_t i = 7;
#endif
  auto from_hex = [](unsigned char c) {
    return c - '0' < 10 ? c - '0' : (c | 32) - 'a' + 10;
  };
  for (; i < raw_uri.size(); i++) {
    if (i + 3 <= raw_uri.size() && raw_uri[i] == '%') {
      ret.push_back(from_hex(raw_uri[i + 1]) * 16 + from_hex(raw_uri[i + 2]));
      i += 2;
    } else
      ret.push_back(raw_uri[i] == '\\' ? '/' : raw_uri[i]);
  }
  return ret;
}

std::string lsPosition::ToString() const {
  return std::to_string(line) + ":" + std::to_string(character);
}

bool lsTextEdit::operator==(const lsTextEdit &that) {
  return range == that.range && newText == that.newText;
}

void Reflect(Writer &visitor, lsMarkedString &value) {
  // If there is a language, emit a `{language:string, value:string}` object. If
  // not, emit a string.
  if (value.language) {
    REFLECT_MEMBER_START();
    REFLECT_MEMBER(language);
    REFLECT_MEMBER(value);
    REFLECT_MEMBER_END();
  } else {
    Reflect(visitor, value.value);
  }
}

std::string Out_ShowLogMessage::method() {
  if (display_type == DisplayType::Log)
    return "window/logMessage";
  return "window/showMessage";
}
