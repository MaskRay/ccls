#include "language_server_api.h"

#include "serializers/json.h"

#include <doctest/doctest.h>
#include <loguru.hpp>

#include <iostream>

MessageRegistry* MessageRegistry::instance_ = nullptr;

lsTextDocumentIdentifier
lsVersionedTextDocumentIdentifier::AsTextDocumentIdentifier() const {
  lsTextDocumentIdentifier result;
  result.uri = uri;
  return result;
}

// Reads a JsonRpc message. |read| returns the next input character.
optional<std::string> ReadJsonRpcContentFrom(
    std::function<optional<char>()> read) {
  // Read the content length. It is terminated by the "\r\n" sequence.
  int exit_seq = 0;
  std::string stringified_content_length;
  while (true) {
    optional<char> opt_c = read();
    if (!opt_c) {
      LOG_S(INFO) << "No more input when reading content length header";
      return nullopt;
    }
    char c = *opt_c;

    if (exit_seq == 0 && c == '\r')
      ++exit_seq;
    if (exit_seq == 1 && c == '\n')
      break;

    stringified_content_length += c;
  }
  const char* kContentLengthStart = "Content-Length: ";
  assert(StartsWith(stringified_content_length, kContentLengthStart));
  int content_length =
      atoi(stringified_content_length.c_str() + strlen(kContentLengthStart));

  // There is always a "\r\n" sequence before the actual content.
  auto expect_char = [&](char expected) {
    optional<char> opt_c = read();
    return opt_c && *opt_c == expected;
  };
  if (!expect_char('\r') || !expect_char('\n')) {
    LOG_S(INFO) << "Unexpected token (expected \r\n sequence)";
    return nullopt;
  }

  // Read content.
  std::string content;
  content.reserve(content_length);
  for (size_t i = 0; i < content_length; ++i) {
    optional<char> c = read();
    if (!c) {
      LOG_S(INFO) << "No more input when reading content body";
      return nullopt;
    }
    content += *c;
  }

  return content;
}

std::function<optional<char>()> MakeContentReader(std::string* content,
                                                  bool can_be_empty) {
  return [content, can_be_empty]() -> optional<char> {
    if (!can_be_empty)
      REQUIRE(!content->empty());
    if (content->empty())
      return nullopt;
    char c = (*content)[0];
    content->erase(content->begin());
    return c;
  };
}

TEST_SUITE("FindIncludeLine") {
  TEST_CASE("ReadContentFromSource") {
    auto parse_correct = [](std::string content) -> std::string {
      auto reader = MakeContentReader(&content, false /*can_be_empty*/);
      auto got = ReadJsonRpcContentFrom(reader);
      REQUIRE(got);
      return got.value();
    };

    auto parse_incorrect = [](std::string content) -> optional<std::string> {
      auto reader = MakeContentReader(&content, true /*can_be_empty*/);
      return ReadJsonRpcContentFrom(reader);
    };

    REQUIRE(parse_correct("Content-Length: 0\r\n\r\n") == "");
    REQUIRE(parse_correct("Content-Length: 1\r\n\r\na") == "a");
    REQUIRE(parse_correct("Content-Length: 4\r\n\r\nabcd") == "abcd");

    REQUIRE(parse_incorrect("ggg") == optional<std::string>());
    REQUIRE(parse_incorrect("Content-Length: 0\r\n") ==
            optional<std::string>());
    REQUIRE(parse_incorrect("Content-Length: 5\r\n\r\nab") ==
            optional<std::string>());
  }
}

optional<char> ReadCharFromStdinBlocking() {
  // Bad stdin means parent process has probably exited. Either way, cquery
  // can no longer be communicated with so just exit.
  if (!std::cin.good()) {
    LOG_S(FATAL) << "std::cin.good() is false; exiting";
    exit(1);
  }

  char c = 0;
  std::cin.read(&c, 1);
  return c;
}

std::unique_ptr<BaseIpcMessage> MessageRegistry::ReadMessageFromStdin(
    bool log_stdin_to_stderr) {
  optional<std::string> content =
      ReadJsonRpcContentFrom(&ReadCharFromStdinBlocking);
  if (!content) {
    LOG_S(FATAL) << "Failed to read JsonRpc input; exiting";
    exit(1);
  }

  if (log_stdin_to_stderr) {
    // TODO: This should go inside of ReadJsonRpcContentFrom since it does not
    // print the header.
    std::string printed = "[CIN] |" + *content + "|\n";
    std::cerr << printed;
    std::cerr.flush();
  }

  rapidjson::Document document;
  document.Parse(content->c_str(), content->length());
  assert(!document.HasParseError());

  JsonReader json_reader{&document};
  return Parse(json_reader);
}

std::unique_ptr<BaseIpcMessage> MessageRegistry::Parse(Reader& visitor) {
  if (!visitor.HasMember("jsonrpc") ||
      std::string(visitor["jsonrpc"]->GetString()) != "2.0") {
    LOG_S(FATAL) << "Bad or missing jsonrpc version";
    exit(1);
  }

  std::string method;
  ReflectMember(visitor, "method", method);

  if (allocators.find(method) == allocators.end()) {
    LOG_S(ERROR) << "Unable to find registered handler for method \"" << method
                 << "\"";
    return nullptr;
  }

  Allocator& allocator = allocators[method];
  return allocator(visitor);
}

MessageRegistry* MessageRegistry::instance() {
  if (!instance_)
    instance_ = new MessageRegistry();

  return instance_;
}

lsBaseOutMessage::~lsBaseOutMessage() = default;

void lsResponseError::Write(Writer& visitor) {
  auto& value = *this;
  int code2 = static_cast<int>(this->code);

  visitor.StartObject();
  REFLECT_MEMBER2("code", code2);
  REFLECT_MEMBER(message);
  if (data) {
    visitor.Key("data");
    data->Write(visitor);
  }
  visitor.EndObject();
}

lsDocumentUri lsDocumentUri::FromPath(const std::string& path) {
  lsDocumentUri result;
  result.SetPath(path);
  return result;
}

lsDocumentUri::lsDocumentUri() {}

bool lsDocumentUri::operator==(const lsDocumentUri& other) const {
  return raw_uri == other.raw_uri;
}

void lsDocumentUri::SetPath(const std::string& path) {
  // file:///c%3A/Users/jacob/Desktop/superindex/indexer/full_tests
  raw_uri = path;

  size_t index = raw_uri.find(":");
  if (index == 1) {  // widows drive letters must always be 1 char
    raw_uri.replace(raw_uri.begin() + index, raw_uri.begin() + index + 1,
                    "%3A");
  }

  // subset of reserved characters from the URI standard
  // http://www.ecma-international.org/ecma-262/6.0/#sec-uri-syntax-and-semantics
  raw_uri = ReplaceAll(raw_uri, " ", "%20");
  raw_uri = ReplaceAll(raw_uri, "(", "%28");
  raw_uri = ReplaceAll(raw_uri, ")", "%29");
  raw_uri = ReplaceAll(raw_uri, "#", "%23");
  raw_uri = ReplaceAll(raw_uri, ",", "%2C");

// TODO: proper fix
#if defined(_WIN32)
  raw_uri = "file:///" + raw_uri;
#else
  raw_uri = "file://" + raw_uri;
#endif
}

std::string lsDocumentUri::GetPath() const {
  // c:/Program%20Files%20%28x86%29/Microsoft%20Visual%20Studio%2014.0/VC/include/vcruntime.
  // C:/Program Files (x86)

  std::string ret;
  if (raw_uri.compare(0, 8, "file:///"))
    return ret;
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

#if defined(_WIN32)
// std::transform(result.begin(), result.end(), result.begin(), ::tolower);
#endif

  return ret;
}

lsPosition::lsPosition() {}
lsPosition::lsPosition(int line, int character)
    : line(line), character(character) {}

bool lsPosition::operator==(const lsPosition& other) const {
  return line == other.line && character == other.character;
}

bool lsPosition::operator<(const lsPosition& other) const {
  return line != other.line ? line < other.line : character < other.character;
}

std::string lsPosition::ToString() const {
  return std::to_string(line) + ":" + std::to_string(character);
}
const lsPosition lsPosition::kZeroPosition = lsPosition();

lsRange::lsRange() {}
lsRange::lsRange(lsPosition start, lsPosition end) : start(start), end(end) {}

bool lsRange::operator==(const lsRange& other) const {
  return start == other.start && end == other.end;
}

lsLocation::lsLocation() {}
lsLocation::lsLocation(lsDocumentUri uri, lsRange range)
    : uri(uri), range(range) {}

bool lsLocation::operator==(const lsLocation& other) const {
  return uri == other.uri && range == other.range;
}

bool lsTextEdit::operator==(const lsTextEdit& that) {
  return range == that.range && newText == that.newText;
}

const std::string& lsCompletionItem::InsertedContent() const {
  if (textEdit)
    return textEdit->newText;
  if (!insertText.empty())
    return insertText;
  return label;
}

void Reflect(Reader& reader, lsInitializeParams::lsTrace& value) {
  if (!reader.IsString()) {
    value = lsInitializeParams::lsTrace::Off;
    return;
  }
  std::string v = reader.GetString();
  if (v == "off")
    value = lsInitializeParams::lsTrace::Off;
  else if (v == "messages")
    value = lsInitializeParams::lsTrace::Messages;
  else if (v == "verbose")
    value = lsInitializeParams::lsTrace::Verbose;
}

void Reflect(Writer& writer, lsInitializeParams::lsTrace& value) {
  switch (value) {
    case lsInitializeParams::lsTrace::Off:
      writer.String("off");
      break;
    case lsInitializeParams::lsTrace::Messages:
      writer.String("messages");
      break;
    case lsInitializeParams::lsTrace::Verbose:
      writer.String("verbose");
      break;
  }
}

std::string Out_ShowLogMessage::method() {
  if (display_type == DisplayType::Log)
    return "window/logMessage";
  return "window/showMessage";
}
