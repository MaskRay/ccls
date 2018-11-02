enum Foo {
#include "funky_enum.h"
};

/*
// TODO: In the future try to have better support for types defined across
// multiple files.

OUTPUT: funky_enum.h
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [],
  "usr2var": [{
      "usr": 439339022761937396,
      "detailed_name": "A",
      "qual_name_offset": 0,
      "short_name": "A",
      "hover": "A = 0",
      "comments": "This file cannot be built directory. It is included in an enum definition of\nanother file.",
      "spell": "4:1-4:2|4:1-4:2|1026|-1",
      "type": 16985894625255407295,
      "kind": 22,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 8524995777615948802,
      "detailed_name": "C",
      "qual_name_offset": 0,
      "short_name": "C",
      "hover": "C = 2",
      "comments": "This file cannot be built directory. It is included in an enum definition of\nanother file.",
      "spell": "6:1-6:2|6:1-6:2|1026|-1",
      "type": 16985894625255407295,
      "kind": 22,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 15962370213938840720,
      "detailed_name": "B",
      "qual_name_offset": 0,
      "short_name": "B",
      "hover": "B = 1",
      "comments": "This file cannot be built directory. It is included in an enum definition of\nanother file.",
      "spell": "5:1-5:2|5:1-5:2|1026|-1",
      "type": 16985894625255407295,
      "kind": 22,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
OUTPUT: funky_enum.cc
{
  "includes": [{
      "line": 1,
      "resolved_path": "&funky_enum.h"
    }],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 16985894625255407295,
      "detailed_name": "enum Foo {}",
      "qual_name_offset": 5,
      "short_name": "Foo",
      "spell": "1:6-1:9|1:1-3:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/