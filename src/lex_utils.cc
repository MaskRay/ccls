#include "lex_utils.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <iostream>

// VSCode (UTF-16) disagrees with Emacs lsp-mode (UTF-8) on how to represent
// text documents.
// We use a UTF-8 iterator to approximate UTF-16 in the specification (weird).
// This is good enough and fails only for UTF-16 surrogate pairs.
int GetOffsetForPosition(lsPosition position, std::string_view content) {
  size_t i = 0;
  for (; position.line > 0 && i < content.size(); i++)
    if (content[i] == '\n')
      position.line--;
  for (; position.character > 0 && i < content.size(); position.character--)
    if (uint8_t(content[i++]) >= 128) {
      // Skip 0b10xxxxxx
      while (i < content.size() && uint8_t(content[i]) >= 128 &&
             uint8_t(content[i]) < 192)
        i++;
    }
  return int(i);
}

lsPosition CharPos(std::string_view search,
                   char character,
                   int character_offset) {
  lsPosition result;
  size_t index = 0;
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

// TODO: eliminate |line_number| param.
optional<lsRange> ExtractQuotedRange(int line_number, const std::string& line) {
  // Find starting and ending quote.
  int start = 0;
  while (start < (int)line.size()) {
    char c = line[start];
    ++start;
    if (c == '"' || c == '<')
      break;
  }
  if (start == (int)line.size())
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

std::string_view LexIdentifierAroundPos(lsPosition position,
                                        std::string_view content) {
  int start = GetOffsetForPosition(position, content);
  int end = start + 1;
  char c;

  // We search for :: before the cursor but not after to get the qualifier.
  for (; start > 0; start--) {
    c = content[start - 1];
    if (isalnum(c) || c == '_')
      ;
    else if (c == ':' && start > 1 && content[start - 2] == ':')
      start--;
    else
      break;
  }

  for (; end < (int)content.size(); end++)
    if (c = content[end], !(isalnum(c) || c == '_'))
      break;

  return content.substr(start, end - start);
}

// Find discontinous |search| in |content|.
// Return |found| and the count of skipped chars before found.
std::pair<bool, int> CaseFoldingSubsequenceMatch(std::string_view search,
                                                 std::string_view content) {
  bool hasUppercaseLetter = std::any_of(search.begin(), search.end(), isupper);
  int skip = 0;
  size_t j = 0;
  for (char c : search) {
    while (j < content.size() &&
           (hasUppercaseLetter ? content[j] != c
                               : tolower(content[j]) != tolower(c)))
      ++j, ++skip;
    if (j == content.size())
      return {false, skip};
    ++j;
  }
  return {true, skip};
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
  TEST_CASE("skip") {
    REQUIRE(CaseFoldingSubsequenceMatch("a", "a") == std::make_pair(true, 0));
    REQUIRE(CaseFoldingSubsequenceMatch("b", "a") == std::make_pair(false, 1));
    REQUIRE(CaseFoldingSubsequenceMatch("", "") == std::make_pair(true, 0));
    REQUIRE(CaseFoldingSubsequenceMatch("a", "ba") == std::make_pair(true, 1));
    REQUIRE(CaseFoldingSubsequenceMatch("aa", "aba") ==
            std::make_pair(true, 1));
    REQUIRE(CaseFoldingSubsequenceMatch("aa", "baa") ==
            std::make_pair(true, 1));
    REQUIRE(CaseFoldingSubsequenceMatch("aA", "aA") == std::make_pair(true, 0));
    REQUIRE(CaseFoldingSubsequenceMatch("aA", "aa") ==
            std::make_pair(false, 1));
    REQUIRE(CaseFoldingSubsequenceMatch("incstdioh", "include <stdio.h>") ==
            std::make_pair(true, 7));
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
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'F'), content) == "Foobar");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'o'), content) == "Foobar");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'b'), content) == "Foobar");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'a'), content) == "Foobar");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'r'), content) == "Foobar");
  }

  TEST_CASE("simple") {
    std::string content = "  Foobar  ";
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'F'), content) == "Foobar");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'o'), content) == "Foobar");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'b'), content) == "Foobar");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'a'), content) == "Foobar");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'r'), content) == "Foobar");
  }

  TEST_CASE("underscores, numbers and ::") {
    std::string content = "  file:ns::_my_t5ype7 ";
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'f'), content) == "file");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 's'), content) == "ns");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, 'y'), content) ==
            "ns::_my_t5ype7");
  }

  TEST_CASE("dot, dash, colon are skipped") {
    std::string content = "1. 2- 3:";
    REQUIRE(LexIdentifierAroundPos(CharPos(content, '1'), content) == "1");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, '2'), content) == "2");
    REQUIRE(LexIdentifierAroundPos(CharPos(content, '3'), content) == "3");
  }
}
