#include "header.h"

void Impl() {
  Foo1<int>();
}

/*
OUTPUT: header.h
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Base",
      "short_name": "Base",
      "qualified_name": "Base",
      "definition_spelling": "10:8-10:12",
      "definition_extent": "10:1-10:15",
      "derived": [1],
      "uses": ["*10:8-10:12", "*12:26-12:30"]
    }, {
      "id": 1,
      "usr": "c:@S@SameFileDerived",
      "short_name": "SameFileDerived",
      "qualified_name": "SameFileDerived",
      "definition_spelling": "12:8-12:23",
      "definition_extent": "12:1-12:33",
      "parents": [0],
      "uses": ["*12:8-12:23", "*14:14-14:29"]
    }, {
      "id": 2,
      "usr": "c:@Foo0",
      "short_name": "Foo0",
      "qualified_name": "Foo0",
      "definition_spelling": "14:7-14:11",
      "definition_extent": "14:1-14:29",
      "alias_of": 1,
      "uses": ["*14:7-14:11"]
    }, {
      "id": 3,
      "usr": "c:@ST>1#T@Foo2",
      "short_name": "Foo2",
      "qualified_name": "Foo2",
      "definition_spelling": "20:8-20:12",
      "definition_extent": "20:1-20:15",
      "uses": ["*20:8-20:12"]
    }, {
      "id": 4,
      "usr": "c:@E@Foo3",
      "short_name": "Foo3",
      "qualified_name": "Foo3",
      "definition_spelling": "22:6-22:10",
      "definition_extent": "22:1-22:22",
      "vars": [0, 1, 2],
      "uses": ["*22:6-22:10"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@FT@>1#TFoo1#v#",
      "short_name": "Foo1",
      "qualified_name": "Foo1",
      "definition_spelling": "17:6-17:10",
      "definition_extent": "17:1-17:15",
      "uses": ["17:6-17:10"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@E@Foo3@A",
      "short_name": "A",
      "qualified_name": "Foo3::A",
      "definition_spelling": "22:13-22:14",
      "definition_extent": "22:13-22:14",
      "variable_type": 4,
      "declaring_type": 4,
      "uses": ["22:13-22:14"]
    }, {
      "id": 1,
      "usr": "c:@E@Foo3@B",
      "short_name": "B",
      "qualified_name": "Foo3::B",
      "definition_spelling": "22:16-22:17",
      "definition_extent": "22:16-22:17",
      "variable_type": 4,
      "declaring_type": 4,
      "uses": ["22:16-22:17"]
    }, {
      "id": 2,
      "usr": "c:@E@Foo3@C",
      "short_name": "C",
      "qualified_name": "Foo3::C",
      "definition_spelling": "22:19-22:20",
      "definition_extent": "22:19-22:20",
      "variable_type": 4,
      "declaring_type": 4,
      "uses": ["22:19-22:20"]
    }, {
      "id": 3,
      "usr": "c:@Foo4",
      "short_name": "Foo4",
      "qualified_name": "Foo4",
      "definition_spelling": "24:5-24:9",
      "definition_extent": "24:1-24:9",
      "uses": ["24:5-24:9"]
    }, {
      "id": 4,
      "usr": "c:header.h@Foo5",
      "short_name": "Foo5",
      "qualified_name": "Foo5",
      "definition_spelling": "25:12-25:16",
      "definition_extent": "25:1-25:16",
      "uses": ["25:12-25:16"]
    }]
}

OUTPUT: impl.cc
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@Impl#",
      "short_name": "Impl",
      "qualified_name": "Impl",
      "definition_spelling": "3:6-3:10",
      "definition_extent": "3:1-5:2",
      "callees": ["1@4:3-4:7"],
      "uses": ["3:6-3:10"]
    }, {
      "id": 1,
      "usr": "c:@FT@>1#TFoo1#v#",
      "callers": ["0@4:3-4:7"],
      "uses": ["4:3-4:7"]
    }]
}
*/
