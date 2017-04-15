#include "simple_header.h"

void impl() {
  header();
}

/*
OUTPUT: simple_header.h
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@header#",
      "short_name": "header",
      "qualified_name": "void header()",
      "declarations": ["3:6-3:12"]
    }]
}
OUTPUT: simple_impl.cc
{
  "dependencies": ["C:/Users/jacob/Desktop/superindex/indexer/tests/multi_file/simple_header.h"],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@impl#",
      "short_name": "impl",
      "qualified_name": "void impl()",
      "definition_spelling": "3:6-3:10",
      "definition_extent": "3:1-5:2",
      "callees": ["1@4:3-4:9"]
    }, {
      "id": 1,
      "usr": "c:@F@header#",
      "callers": ["0@4:3-4:9"]
    }]
}
*/
