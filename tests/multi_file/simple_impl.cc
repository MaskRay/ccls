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
      "detailed_name": "void header()",
      "declarations": [{
          "spelling": "3:6-3:12",
          "extent": "3:1-3:14",
          "content": "void header()"
        }]
    }]
}
OUTPUT: simple_impl.cc
{
  "includes": [{
      "line": 1,
      "resolved_path": "C:/Users/jacob/Desktop/cquery/tests/multi_file/simple_header.h"
    }],
  "dependencies": ["C:/Users/jacob/Desktop/cquery/tests/multi_file/simple_header.h"],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@impl#",
      "short_name": "impl",
      "detailed_name": "void impl()",
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
