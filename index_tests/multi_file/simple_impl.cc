#include "simple_header.h"

void impl() {
  header();
}

/*
OUTPUT: simple_header.h
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 16236105532929924676,
      "detailed_name": "void header()",
      "qual_name_offset": 5,
      "short_name": "header",
      "kind": 12,
      "storage": 0,
      "declarations": ["3:6-3:12|0|1|1"],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
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
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 3373269392705484958,
      "detailed_name": "void impl()",
      "qual_name_offset": 5,
      "short_name": "impl",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "3:6-3:10|0|1|2",
      "extent": "3:1-5:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["4:3-4:9|16236105532929924676|3|32"]
    }, {
      "usr": 16236105532929924676,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["4:3-4:9|3373269392705484958|3|32"],
      "callees": []
    }],
  "usr2type": [],
  "usr2var": []
}
*/
