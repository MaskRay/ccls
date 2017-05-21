enum Foo {
#include "funky_enum.h"
};

/*
// TODO: In the future try to have better support for types defined across
// multiple files.

OUTPUT: funky_enum.h
{
  "types": [{
      "id": 0,
      "usr": "c:@E@Foo",
      "vars": [0, 1, 2]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@E@Foo@A",
      "short_name": "A",
      "detailed_name": "Foo Foo::A",
      "definition_spelling": "4:1-4:2",
      "definition_extent": "4:1-4:2",
      "variable_type": 0,
      "declaring_type": 0,
      "is_local": false,
      "uses": ["4:1-4:2"]
    }, {
      "id": 1,
      "usr": "c:@E@Foo@B",
      "short_name": "B",
      "detailed_name": "Foo Foo::B",
      "definition_spelling": "5:1-5:2",
      "definition_extent": "5:1-5:2",
      "variable_type": 0,
      "declaring_type": 0,
      "is_local": false,
      "uses": ["5:1-5:2"]
    }, {
      "id": 2,
      "usr": "c:@E@Foo@C",
      "short_name": "C",
      "detailed_name": "Foo Foo::C",
      "definition_spelling": "6:1-6:2",
      "definition_extent": "6:1-6:2",
      "variable_type": 0,
      "declaring_type": 0,
      "is_local": false,
      "uses": ["6:1-6:2"]
    }]
}
OUTPUT: funky_enum.cc
{
  "includes": [{
      "line": 2,
      "resolved_path": "C:/Users/jacob/Desktop/superindex/indexer/tests/multi_file/funky_enum.h"
    }],
  "dependencies": ["C:/Users/jacob/Desktop/superindex/indexer/tests/multi_file/funky_enum.h"],
  "types": [{
      "id": 0,
      "usr": "c:@E@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-3:2",
      "uses": ["1:6-1:9"]
    }]
}
*/