#include "lex_utils.h"

#include <doctest/doctest.h>

#include <algorithm>

int GetOffsetForPosition(lsPosition position, const std::string& content) {
  if (content.empty())
    return 0;

  int offset = 0;

  int remaining_lines = position.line;
  while (remaining_lines > 0 && offset < static_cast<int>(content.size())) {
    if (content[offset] == '\n')
      --remaining_lines;
    ++offset;
  }

  return std::min<int>(offset + position.character, content.size());
}

lsPosition CharPos(const std::string& search,
                   char character,
                   int character_offset) {
  lsPosition result;
  int index = 0;
  while (index < search.size()) {
    char c = search[index];
    if (c == character)
      break;
    if (c == '\n') {
      result.line += 1;
      result.character = 0;
    } else {
      result.character += 1;
    }
    ++index;
  }
  assert(index < search.size());
  result.character += character_offset;
  return result;
}

bool ShouldRunIncludeCompletion(const std::string& line) {
  size_t start = 0;
  while (start < line.size() && isspace(line[start]))
    ++start;
  return start < line.size() && line[start] == '#';
}

// TODO: eliminate |line_number| param.
optional<lsRange> ExtractQuotedRange(int line_number, const std::string& line) {
  // Find starting and ending quote.
  int start = 0;
  while (start < line.size()) {
    char c = line[start];
    ++start;
    if (c == '"' || c == '<')
      break;
  }
  if (start == line.size())
    return nullopt;

  int end = (int)line.size();
  while (end > 0) {
    char c = line[end];
    if (c == '"' || c == '>')
      break;
    --end;
  }

  if (start >= end)
    return nullopt;

  return lsRange(lsPosition(line_number, start), lsPosition(line_number, end));
}

void LexFunctionDeclaration(const std::string& buffer_content,
                            lsPosition declaration_spelling,
                            optional<std::string> type_name,
                            std::string* insert_text,
                            int* newlines_after_name) {
  int name_start = GetOffsetForPosition(declaration_spelling, buffer_content);

  bool parse_return_type = true;
  // We need to check if we have a return type (ctors and dtors do not).
  if (type_name) {
    int name_end = name_start;
    while (name_end < buffer_content.size()) {
      char c = buffer_content[name_end];
      if (isspace(c) || c == '(')
        break;
      ++name_end;
    }

    std::string func_name =
        buffer_content.substr(name_start, name_end - name_start);
    if (func_name == *type_name || func_name == ("~" + *type_name))
      parse_return_type = false;
  }

  // We need to fetch the return type. This can get complex, ie,
  //
  //  std::vector <int> foo();
  //
  int return_start = name_start;
  if (parse_return_type) {
    int paren_balance = 0;
    int angle_balance = 0;
    bool expect_token = true;
    while (return_start > 0) {
      char c = buffer_content[return_start - 1];
      if (paren_balance == 0 && angle_balance == 0) {
        if (isspace(c) && !expect_token) {
          break;
        }
        if (!isspace(c))
          expect_token = false;
      }

      if (c == ')')
        ++paren_balance;
      if (c == '(') {
        --paren_balance;
        expect_token = true;
      }

      if (c == '>')
        ++angle_balance;
      if (c == '<') {
        --angle_balance;
        expect_token = true;
      }

      return_start -= 1;
    }
  }

  // We need to fetch the arguments. Just scan for the next ';'.
  *newlines_after_name = 0;
  int end = name_start;
  while (end < buffer_content.size()) {
    char c = buffer_content[end];
    if (c == ';')
      break;
    if (c == '\n')
      *newlines_after_name += 1;
    ++end;
  }

  std::string result;
  result += buffer_content.substr(return_start, name_start - return_start);
  if (type_name && !type_name->empty())
    result += *type_name + "::";
  result += buffer_content.substr(name_start, end - name_start);
  TrimEndInPlace(result);
  result += " {\n}";
  *insert_text = result;
}

std::string LexWordAroundPos(lsPosition position, const std::string& content) {
  int index = GetOffsetForPosition(position, content);

  int start = index;
  int end = index;

  while (start > 0) {
    char c = content[start - 1];
    if (isalnum(c) || c == '_') {
      --start;
    } else {
      break;
    }
  }

  while ((end + 1) < content.size()) {
    char c = content[end + 1];
    if (isalnum(c) || c == '_') {
      ++end;
    } else {
      break;
    }
  }

  return content.substr(start, end - start + 1);
}

bool SubstringMatch(const std::string& search, const std::string& content) {
  if (search.empty())
    return true;

  size_t search_index = 0;
  char search_char = tolower(search[search_index]);

  size_t content_index = 0;

  while (true) {
    char content_char = tolower(content[content_index]);

    if (content_char == search_char) {
      search_index += 1;
      if (search_index >= search.size())
        return true;
      search_char = tolower(search[search_index]);
    }

    content_index += 1;
    if (content_index >= content.size())
      return false;
  }

  return false;
}

TEST_SUITE("Offset") {
  TEST_CASE("past end") {
    std::string content = "foo";
    int offset = GetOffsetForPosition(lsPosition(10, 10), content);
    REQUIRE(offset <= content.size());
  }

  TEST_CASE("in middle of content") {
    std::string content = "abcdefghijk";
    for (int i = 0; i < content.size(); ++i) {
      int offset = GetOffsetForPosition(lsPosition(0, i), content);
      REQUIRE(i == offset);
    }
  }

  TEST_CASE("at end of content") {
    REQUIRE(GetOffsetForPosition(lsPosition(0, 0), "") == 0);
    REQUIRE(GetOffsetForPosition(lsPosition(0, 1), "a") == 1);
  }
}

TEST_SUITE("Substring") {
  TEST_CASE("match") {
    // Sanity.
    REQUIRE(SubstringMatch("a", "aa"));
    REQUIRE(SubstringMatch("aa", "aa"));

    // Empty string matches anything.
    REQUIRE(SubstringMatch("", ""));
    REQUIRE(SubstringMatch("", "aa"));

    // Match in start/middle/end.
    REQUIRE(SubstringMatch("a", "abbbb"));
    REQUIRE(SubstringMatch("a", "bbabb"));
    REQUIRE(SubstringMatch("a", "bbbba"));
    REQUIRE(SubstringMatch("aa", "aabbb"));
    REQUIRE(SubstringMatch("aa", "bbaab"));
    REQUIRE(SubstringMatch("aa", "bbbaa"));

    // Capitalization.
    REQUIRE(SubstringMatch("aa", "aA"));
    REQUIRE(SubstringMatch("aa", "Aa"));
    REQUIRE(SubstringMatch("aa", "AA"));

    // Token skipping.
    REQUIRE(SubstringMatch("ad", "abcd"));
    REQUIRE(SubstringMatch("ad", "ABCD"));

    // Ordering.
    REQUIRE(!SubstringMatch("ad", "dcba"));
  }
}

TEST_SUITE("LexFunctionDeclaration") {
  TEST_CASE("simple") {
    std::string buffer_content = " void Foo(); ";
    lsPosition declaration = CharPos(buffer_content, 'F');
    std::string insert_text;
    int newlines_after_name = 0;

    LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text,
                           &newlines_after_name);
    REQUIRE(insert_text == "void Foo() {\n}");
    REQUIRE(newlines_after_name == 0);

    LexFunctionDeclaration(buffer_content, declaration, std::string("Type"),
                           &insert_text, &newlines_after_name);
    REQUIRE(insert_text == "void Type::Foo() {\n}");
    REQUIRE(newlines_after_name == 0);
  }

  TEST_CASE("ctor") {
    std::string buffer_content = " Foo(); ";
    lsPosition declaration = CharPos(buffer_content, 'F');
    std::string insert_text;
    int newlines_after_name = 0;

    LexFunctionDeclaration(buffer_content, declaration, std::string("Foo"),
                           &insert_text, &newlines_after_name);
    REQUIRE(insert_text == "Foo::Foo() {\n}");
    REQUIRE(newlines_after_name == 0);
  }

  TEST_CASE("dtor") {
    std::string buffer_content = " ~Foo(); ";
    lsPosition declaration = CharPos(buffer_content, '~');
    std::string insert_text;
    int newlines_after_name = 0;

    LexFunctionDeclaration(buffer_content, declaration, std::string("Foo"),
                           &insert_text, &newlines_after_name);
    REQUIRE(insert_text == "Foo::~Foo() {\n}");
    REQUIRE(newlines_after_name == 0);
  }

  TEST_CASE("complex return type") {
    std::string buffer_content = " std::vector<int> Foo(); ";
    lsPosition declaration = CharPos(buffer_content, 'F');
    std::string insert_text;
    int newlines_after_name = 0;

    LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text,
                           &newlines_after_name);
    REQUIRE(insert_text == "std::vector<int> Foo() {\n}");
    REQUIRE(newlines_after_name == 0);

    LexFunctionDeclaration(buffer_content, declaration, std::string("Type"),
                           &insert_text, &newlines_after_name);
    REQUIRE(insert_text == "std::vector<int> Type::Foo() {\n}");
    REQUIRE(newlines_after_name == 0);
  }

  TEST_CASE("extra complex return type") {
    std::string buffer_content = " std::function < int() > \n Foo(); ";
    lsPosition declaration = CharPos(buffer_content, 'F');
    std::string insert_text;
    int newlines_after_name = 0;

    LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text,
                           &newlines_after_name);
    REQUIRE(insert_text == "std::function < int() > \n Foo() {\n}");
    REQUIRE(newlines_after_name == 0);

    LexFunctionDeclaration(buffer_content, declaration, std::string("Type"),
                           &insert_text, &newlines_after_name);
    REQUIRE(insert_text == "std::function < int() > \n Type::Foo() {\n}");
    REQUIRE(newlines_after_name == 0);
  }

  TEST_CASE("parameters") {
    std::string buffer_content = "void Foo(int a,\n\n    int b); ";
    lsPosition declaration = CharPos(buffer_content, 'F');
    std::string insert_text;
    int newlines_after_name = 0;

    LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text,
                           &newlines_after_name);
    REQUIRE(insert_text == "void Foo(int a,\n\n    int b) {\n}");
    REQUIRE(newlines_after_name == 2);

    LexFunctionDeclaration(buffer_content, declaration, std::string("Type"),
                           &insert_text, &newlines_after_name);
    REQUIRE(insert_text == "void Type::Foo(int a,\n\n    int b) {\n}");
    REQUIRE(newlines_after_name == 2);
  }
}

TEST_SUITE("LexWordAroundPos") {
  TEST_CASE("edges") {
    std::string content = "Foobar";
    REQUIRE(LexWordAroundPos(CharPos(content, 'F'), content) == "Foobar");
    REQUIRE(LexWordAroundPos(CharPos(content, 'o'), content) == "Foobar");
    REQUIRE(LexWordAroundPos(CharPos(content, 'b'), content) == "Foobar");
    REQUIRE(LexWordAroundPos(CharPos(content, 'a'), content) == "Foobar");
    REQUIRE(LexWordAroundPos(CharPos(content, 'r'), content) == "Foobar");
  }

  TEST_CASE("simple") {
    std::string content = "  Foobar  ";
    REQUIRE(LexWordAroundPos(CharPos(content, 'F'), content) == "Foobar");
    REQUIRE(LexWordAroundPos(CharPos(content, 'o'), content) == "Foobar");
    REQUIRE(LexWordAroundPos(CharPos(content, 'b'), content) == "Foobar");
    REQUIRE(LexWordAroundPos(CharPos(content, 'a'), content) == "Foobar");
    REQUIRE(LexWordAroundPos(CharPos(content, 'r'), content) == "Foobar");
  }

  TEST_CASE("underscores and numbers") {
    std::string content = "  _my_t5ype7  ";
    REQUIRE(LexWordAroundPos(CharPos(content, '_'), content) == "_my_t5ype7");
    REQUIRE(LexWordAroundPos(CharPos(content, '5'), content) == "_my_t5ype7");
    REQUIRE(LexWordAroundPos(CharPos(content, 'e'), content) == "_my_t5ype7");
    REQUIRE(LexWordAroundPos(CharPos(content, '7'), content) == "_my_t5ype7");
  }

  TEST_CASE("dot, dash, colon are skipped") {
    std::string content = "1. 2- 3:";
    REQUIRE(LexWordAroundPos(CharPos(content, '1'), content) == "1");
    REQUIRE(LexWordAroundPos(CharPos(content, '2'), content) == "2");
    REQUIRE(LexWordAroundPos(CharPos(content, '3'), content) == "3");
  }
}
