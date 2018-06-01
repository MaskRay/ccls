#include "static.h"

void Buffer::CreateSharedBuffer() {}

/*
OUTPUT: static.h
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 14576076421851654759,
      "detailed_name": "static void Buffer::CreateSharedBuffer()",
      "qual_name_offset": 12,
      "short_name": "CreateSharedBuffer",
      "kind": 254,
      "storage": 2,
      "declarations": ["4:15-4:33|9411323049603567600|2|1"],
      "declaring_type": 9411323049603567600,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 9411323049603567600,
      "detailed_name": "Buffer",
      "qual_name_offset": 0,
      "short_name": "Buffer",
      "kind": 23,
      "declarations": [],
      "spell": "3:8-3:14|0|1|2",
      "extent": "3:1-5:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [14576076421851654759],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
OUTPUT: static.cc
{
  "includes": [{
      "line": 0,
      "resolved_path": "&static.h"
    }],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 14576076421851654759,
      "detailed_name": "void Buffer::CreateSharedBuffer()",
      "qual_name_offset": 5,
      "short_name": "CreateSharedBuffer",
      "kind": 254,
      "storage": 0,
      "declarations": [],
      "spell": "3:14-3:32|9411323049603567600|2|2",
      "extent": "3:1-3:37|0|1|0",
      "declaring_type": 9411323049603567600,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 9411323049603567600,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [14576076421851654759],
      "vars": [],
      "instances": [],
      "uses": ["3:6-3:12|0|1|4"]
    }],
  "usr2var": []
}
*/