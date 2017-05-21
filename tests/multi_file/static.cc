#include "static.h"

void Buffer::CreateSharedBuffer() {}

/*
OUTPUT: static.h
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Buffer",
      "short_name": "Buffer",
      "detailed_name": "Buffer",
      "definition_spelling": "3:8-3:14",
      "definition_extent": "3:1-5:2",
      "funcs": [0],
      "uses": ["3:8-3:14"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Buffer@F@CreateSharedBuffer#S",
      "short_name": "CreateSharedBuffer",
      "detailed_name": "void Buffer::CreateSharedBuffer()",
      "declarations": ["4:15-4:33"],
      "declaring_type": 0
    }]
}
OUTPUT: static.cc
{
  "includes": [{
      "line": 1,
      "resolved_path": "C:/Users/jacob/Desktop/superindex/indexer/tests/multi_file/static.h"
    }],
  "dependencies": ["C:/Users/jacob/Desktop/superindex/indexer/tests/multi_file/static.h"],
  "types": [{
      "id": 0,
      "usr": "c:@S@Buffer",
      "funcs": [0],
      "uses": ["3:6-3:12"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Buffer@F@CreateSharedBuffer#S",
      "short_name": "CreateSharedBuffer",
      "detailed_name": "void Buffer::CreateSharedBuffer()",
      "definition_spelling": "3:14-3:32",
      "definition_extent": "3:1-3:37",
      "declaring_type": 0
    }]
}
*/