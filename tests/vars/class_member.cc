class Foo {
  Foo* member;
};
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "hover": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-3:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0],
      "instances": [0],
      "uses": ["1:7-1:10", "2:3-2:6"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@member",
      "short_name": "member",
      "detailed_name": "Foo * Foo::member",
      "hover": "Foo *",
      "definition_spelling": "2:8-2:14",
      "definition_extent": "2:3-2:14",
      "variable_type": 0,
      "declaring_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["2:8-2:14"]
    }]
}
*/
