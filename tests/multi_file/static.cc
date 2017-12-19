#include "static.h"

void Buffer::CreateSharedBuffer() {}

/*
OUTPUT: static.h
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Buffer",
      "short_name": "Buffer",
      "detailed_name": "Buffer",
      "hover": "Buffer",
      "definition_spelling": "3:8-3:14",
      "definition_extent": "3:1-5:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["3:8-3:14"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Buffer@F@CreateSharedBuffer#S",
      "short_name": "CreateSharedBuffer",
      "detailed_name": "void Buffer::CreateSharedBuffer()",
      "hover": "void Buffer::CreateSharedBuffer()",
      "declarations": [{
          "spelling": "4:15-4:33",
          "extent": "4:3-4:35",
          "content": "static void CreateSharedBuffer()",
          "param_spellings": []
        }],
      "declaring_type": 0,
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
OUTPUT: static.cc
{
  "includes": [{
      "line": 1,
      "resolved_path": "&static.h"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Buffer",
      "short_name": "",
      "detailed_name": "",
      "hover": "",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["3:6-3:12"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Buffer@F@CreateSharedBuffer#S",
      "short_name": "CreateSharedBuffer",
      "detailed_name": "void Buffer::CreateSharedBuffer()",
      "hover": "void Buffer::CreateSharedBuffer()",
      "declarations": [],
      "definition_spelling": "3:14-3:32",
      "definition_extent": "3:1-3:37",
      "declaring_type": 0,
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
*/