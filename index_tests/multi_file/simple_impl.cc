#include "simple_header.h"

void impl() {
  header();
}

/*
OUTPUT: simple_header.h
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 16236105532929924676,
      "detailed_name": "void header()",
      "qual_name_offset": 5,
      "short_name": "header",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["3:6-3:12|3:1-3:14|1|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [],
  "usr2var": []
}
OUTPUT: simple_impl.cc
{
  "includes": [{
      "line": 0,
      "resolved_path": "&simple_header.h"
    }],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3373269392705484958,
      "detailed_name": "void impl()",
      "qual_name_offset": 5,
      "short_name": "impl",
      "spell": "3:6-3:10|3:1-5:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["4:3-4:9|16236105532929924676|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 16236105532929924676,
      "detailed_name": "void header()",
      "qual_name_offset": 5,
      "short_name": "header",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["4:3-4:9|16420|-1"]
    }],
  "usr2type": [],
  "usr2var": []
}
*/
