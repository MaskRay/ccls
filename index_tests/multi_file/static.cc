#include "static.h"

void Buffer::CreateSharedBuffer() {}

/*
OUTPUT: static.h
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 14576076421851654759,
      "detailed_name": "static void Buffer::CreateSharedBuffer()",
      "qual_name_offset": 12,
      "short_name": "CreateSharedBuffer",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 254,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["4:15-4:33|4:3-4:35|1025|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 9411323049603567600,
      "detailed_name": "struct Buffer {}",
      "qual_name_offset": 7,
      "short_name": "Buffer",
      "spell": "3:8-3:14|3:1-5:2|2|-1",
      "bases": [],
      "funcs": [14576076421851654759],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 14576076421851654759,
      "detailed_name": "static void Buffer::CreateSharedBuffer()",
      "qual_name_offset": 12,
      "short_name": "CreateSharedBuffer",
      "spell": "3:14-3:32|3:1-3:37|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 254,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 9411323049603567600,
      "detailed_name": "struct Buffer {}",
      "qual_name_offset": 7,
      "short_name": "Buffer",
      "bases": [],
      "funcs": [14576076421851654759],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["3:6-3:12|4|-1"]
    }],
  "usr2var": []
}
*/