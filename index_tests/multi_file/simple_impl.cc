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
  "includes": [],
  "skipped_ranges": [],
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
      "callees": []
    }, {
      "usr": 16236105532929924676,
      "detailed_name": "void header()",
      "qual_name_offset": 5,
      "short_name": "header",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["4:3-4:9|0|1|8228"],
      "callees": []
    }],
  "usr2type": [],
  "usr2var": []
}
*/
