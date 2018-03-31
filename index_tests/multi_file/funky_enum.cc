enum Foo {
#include "funky_enum.h"
};

/*
// TODO: In the future try to have better support for types defined across
// multiple files.

OUTPUT: funky_enum.h
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 16985894625255407295,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 439339022761937396,
      "detailed_name": "Foo::A",
      "short_name": "A",
      "hover": "Foo::A = 0",
      "declarations": [],
      "spell": "4:1-4:2|0|2|2",
      "extent": "4:1-4:2|0|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "id": 1,
      "usr": 15962370213938840720,
      "detailed_name": "Foo::B",
      "short_name": "B",
      "hover": "Foo::B = 1",
      "declarations": [],
      "spell": "5:1-5:2|0|2|2",
      "extent": "5:1-5:2|0|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "id": 2,
      "usr": 8524995777615948802,
      "detailed_name": "Foo::C",
      "short_name": "C",
      "hover": "Foo::C = 2",
      "declarations": [],
      "spell": "6:1-6:2|0|2|2",
      "extent": "6:1-6:2|0|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }]
}
OUTPUT: funky_enum.cc
{
  "includes": [{
      "line": 1,
      "resolved_path": "&funky_enum.h"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 16985894625255407295,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 10,
      "declarations": [],
      "spell": "1:6-1:9|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [],
  "vars": []
}
*/