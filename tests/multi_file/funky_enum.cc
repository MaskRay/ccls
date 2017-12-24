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
      "usr": "c:@E@Foo",
      "short_name": "",
      "detailed_name": "",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1, 2],
      "instances": [],
      "uses": []
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:@E@Foo@A",
      "short_name": "A",
      "detailed_name": "Foo::A",
      "definition_spelling": "4:1-4:2",
      "definition_extent": "4:1-4:2",
      "variable_type": 0,
      "declaring_type": 0,
      "cls": 4,
      "uses": ["4:1-4:2"]
    }, {
      "id": 1,
      "usr": "c:@E@Foo@B",
      "short_name": "B",
      "detailed_name": "Foo::B",
      "definition_spelling": "5:1-5:2",
      "definition_extent": "5:1-5:2",
      "variable_type": 0,
      "declaring_type": 0,
      "cls": 4,
      "uses": ["5:1-5:2"]
    }, {
      "id": 2,
      "usr": "c:@E@Foo@C",
      "short_name": "C",
      "detailed_name": "Foo::C",
      "definition_spelling": "6:1-6:2",
      "definition_extent": "6:1-6:2",
      "variable_type": 0,
      "declaring_type": 0,
      "cls": 4,
      "uses": ["6:1-6:2"]
    }]
}
OUTPUT: funky_enum.cc
{
  "includes": [{
      "line": 2,
      "resolved_path": "&funky_enum.h"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@E@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-3:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:6-1:9"]
    }],
  "funcs": [],
  "vars": []
}
*/