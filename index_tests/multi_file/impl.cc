#include "header.h"

void Impl() {
  Foo1<int>();
}

/*
OUTPUT: header.h
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 8420119006782424779,
      "detailed_name": "Base",
      "short_name": "Base",
      "kind": 23,
      "declarations": ["5:26-5:30|-1|1|4"],
      "spell": "3:8-3:12|-1|1|2",
      "extent": "3:1-3:15|-1|1|0",
      "bases": [],
      "derived": [1],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["5:26-5:30|-1|1|4"]
    }, {
      "id": 1,
      "usr": 16750616846959666305,
      "detailed_name": "SameFileDerived",
      "short_name": "SameFileDerived",
      "kind": 23,
      "declarations": [],
      "spell": "5:8-5:23|-1|1|2",
      "extent": "5:1-5:33|-1|1|0",
      "bases": [0],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:14-7:29|-1|1|4"]
    }, {
      "id": 2,
      "usr": 619345544228965342,
      "detailed_name": "Foo0",
      "short_name": "Foo0",
      "kind": 252,
      "hover": "using Foo0 = SameFileDerived",
      "declarations": [],
      "spell": "7:7-7:11|-1|1|2",
      "extent": "7:1-7:29|-1|1|0",
      "alias_of": 1,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:7-7:11|-1|1|4"]
    }, {
      "id": 3,
      "usr": 529393482671181129,
      "detailed_name": "Foo2",
      "short_name": "Foo2",
      "kind": 5,
      "declarations": [],
      "spell": "13:8-13:12|-1|1|2",
      "extent": "13:1-13:15|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 4,
      "usr": 4481210672785600703,
      "detailed_name": "Foo3",
      "short_name": "Foo3",
      "kind": 10,
      "declarations": [],
      "spell": "15:6-15:10|-1|1|2",
      "extent": "15:1-15:22|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 5,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [3, 4],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 11650481237659640387,
      "detailed_name": "void Foo1()",
      "short_name": "Foo1",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "10:6-10:10|-1|1|2",
      "extent": "10:1-10:15|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 6141718166919284735,
      "detailed_name": "Foo3::A",
      "short_name": "A",
      "hover": "Foo3::A = 0",
      "declarations": [],
      "spell": "15:13-15:14|4|2|2",
      "extent": "15:13-15:14|4|2|0",
      "type": 4,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "id": 1,
      "usr": 17716334512218775320,
      "detailed_name": "Foo3::B",
      "short_name": "B",
      "hover": "Foo3::B = 1",
      "declarations": [],
      "spell": "15:16-15:17|4|2|2",
      "extent": "15:16-15:17|4|2|0",
      "type": 4,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "id": 2,
      "usr": 7285646116511901840,
      "detailed_name": "Foo3::C",
      "short_name": "C",
      "hover": "Foo3::C = 2",
      "declarations": [],
      "spell": "15:19-15:20|4|2|2",
      "extent": "15:19-15:20|4|2|0",
      "type": 4,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "id": 3,
      "usr": 2638219001294786365,
      "detailed_name": "int Foo4",
      "short_name": "Foo4",
      "declarations": [],
      "spell": "17:5-17:9|-1|1|2",
      "extent": "17:1-17:9|-1|1|0",
      "type": 5,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 4,
      "usr": 8395885290297540138,
      "detailed_name": "int Foo5",
      "short_name": "Foo5",
      "declarations": [],
      "spell": "18:12-18:16|-1|1|2",
      "extent": "18:1-18:16|-1|1|0",
      "type": 5,
      "uses": [],
      "kind": 13,
      "storage": 3
    }]
}
OUTPUT: impl.cc
{
  "includes": [{
      "line": 0,
      "resolved_path": "&header.h"
    }],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "usr": 5817708529036841195,
      "detailed_name": "void Impl()",
      "short_name": "Impl",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:6-3:10|-1|1|2",
      "extent": "3:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["4:3-4:7|1|3|32"]
    }, {
      "id": 1,
      "usr": 11650481237659640387,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["4:3-4:7|0|3|32"],
      "callees": []
    }],
  "vars": []
}
*/
