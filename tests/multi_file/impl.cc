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
      "definition_spelling": "3:8-3:12",
      "definition_extent": "3:1-3:15",
      "derived": [1],
      "uses": ["*3:8-3:12", "*5:26-5:30"]
    }, {
      "id": 1,
      "usr": "c:@S@SameFileDerived",
      "short_name": "SameFileDerived",
      "qualified_name": "SameFileDerived",
      "definition_spelling": "5:8-5:23",
      "definition_extent": "5:1-5:33",
      "parents": [0],
      "uses": ["*5:8-5:23", "*7:14-7:29"]
    }, {
      "id": 2,
      "usr": "c:@Foo0",
      "short_name": "Foo0",
      "qualified_name": "Foo0",
      "definition_spelling": "7:7-7:11",
      "definition_extent": "7:1-7:29",
      "alias_of": 1,
      "uses": ["*7:7-7:11"]
    }, {
      "id": 3,
      "usr": "c:@ST>1#T@Foo2",
      "short_name": "Foo2",
      "qualified_name": "Foo2",
      "definition_spelling": "13:8-13:12",
      "definition_extent": "13:1-13:15",
      "uses": ["*13:8-13:12"]
    }, {
      "id": 4,
      "usr": "c:@E@Foo3",
      "short_name": "Foo3",
      "qualified_name": "Foo3",
      "definition_spelling": "15:6-15:10",
      "definition_extent": "15:1-15:22",
      "vars": [0, 1, 2],
      "uses": ["*15:6-15:10"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@FT@>1#TFoo1#v#",
      "short_name": "Foo1",
      "qualified_name": "Foo1",
      "definition_spelling": "10:6-10:10",
      "definition_extent": "10:1-10:15",
      "uses": ["10:6-10:10"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@E@Foo3@A",
      "short_name": "A",
      "qualified_name": "Foo3::A",
      "definition_spelling": "15:13-15:14",
      "definition_extent": "15:13-15:14",
      "variable_type": 4,
      "declaring_type": 4,
      "uses": ["15:13-15:14"]
    }, {
      "id": 1,
      "usr": "c:@E@Foo3@B",
      "short_name": "B",
      "qualified_name": "Foo3::B",
      "definition_spelling": "15:16-15:17",
      "definition_extent": "15:16-15:17",
      "variable_type": 4,
      "declaring_type": 4,
      "uses": ["15:16-15:17"]
    }, {
      "id": 2,
      "usr": "c:@E@Foo3@C",
      "short_name": "C",
      "qualified_name": "Foo3::C",
      "definition_spelling": "15:19-15:20",
      "definition_extent": "15:19-15:20",
      "variable_type": 4,
      "declaring_type": 4,
      "uses": ["15:19-15:20"]
    }, {
      "id": 3,
      "usr": "c:@Foo4",
      "short_name": "Foo4",
      "qualified_name": "Foo4",
      "definition_spelling": "17:5-17:9",
      "definition_extent": "17:1-17:9",
      "uses": ["17:5-17:9"]
    }, {
      "id": 4,
      "usr": "c:header.h@Foo5",
      "short_name": "Foo5",
      "qualified_name": "Foo5",
      "definition_spelling": "18:12-18:16",
      "definition_extent": "18:1-18:16",
      "uses": ["18:12-18:16"]
    }]
}

OUTPUT: impl.cc
{
  "dependencies": ["C:/Users/jacob/Desktop/superindex/indexer/tests/multi_file/header.h"],
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
