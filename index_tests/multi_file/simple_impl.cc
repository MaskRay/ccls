#include "simple_header.h"

void impl() {
  header();
}

/*
OUTPUT: simple_header.h
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "usr": 16236105532929924676,
      "detailed_name": "void header()",
      "short_name": "header",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "3:6-3:12|-1|1|1",
          "param_spellings": []
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
OUTPUT: simple_impl.cc
{
  "includes": [{
      "line": 0,
      "resolved_path": "&simple_header.h"
    }],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "usr": 3373269392705484958,
      "detailed_name": "void impl()",
      "short_name": "impl",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:6-3:10|-1|1|2",
      "extent": "3:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["4:3-4:9|1|3|32"]
    }, {
      "id": 1,
      "usr": 16236105532929924676,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["4:3-4:9|0|3|32"],
      "callees": []
    }],
  "vars": []
}
*/
