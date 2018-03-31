#include "static.h"

void Buffer::CreateSharedBuffer() {}

/*
OUTPUT: static.h
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 9411323049603567600,
      "detailed_name": "Buffer",
      "short_name": "Buffer",
      "kind": 23,
      "declarations": [],
      "spell": "3:8-3:14|-1|1|2",
      "extent": "3:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 14576076421851654759,
      "detailed_name": "void Buffer::CreateSharedBuffer()",
      "short_name": "CreateSharedBuffer",
      "kind": 254,
      "storage": 3,
      "declarations": [{
          "spell": "4:15-4:33|0|2|1",
          "param_spellings": []
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
OUTPUT: static.cc
{
  "includes": [{
      "line": 0,
      "resolved_path": "&static.h"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 9411323049603567600,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["3:6-3:12|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 14576076421851654759,
      "detailed_name": "void Buffer::CreateSharedBuffer()",
      "short_name": "CreateSharedBuffer",
      "kind": 254,
      "storage": 1,
      "declarations": [],
      "spell": "3:14-3:32|0|2|2",
      "extent": "3:1-3:37|-1|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/