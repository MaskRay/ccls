struct S {};
using F = S;
void Foo() {
  F a;
}

// TODO: Should we also add a usage to |S|?

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@S",
      "short_name": "S",
      "detailed_name": "S",
      "definition_spelling": "1:8-1:9",
      "definition_extent": "1:1-1:12",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:8-1:9", "2:11-2:12"]
    }, {
      "id": 1,
      "usr": "c:@F",
      "short_name": "F",
      "detailed_name": "using F = S",
      "definition_spelling": "2:7-2:8",
      "definition_extent": "2:1-2:12",
      "alias_of": 0,
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["2:7-2:8", "4:3-4:4"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@Foo#",
      "short_name": "Foo",
      "detailed_name": "void Foo()",
      "declarations": [],
      "definition_spelling": "3:6-3:9",
      "definition_extent": "3:1-5:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_instance_on_using_type.cc@44@F@Foo#@a",
      "short_name": "a",
      "detailed_name": "F a",
      "definition_spelling": "4:5-4:6",
      "definition_extent": "4:3-4:6",
      "variable_type": 1,
      "is_local": true,
      "is_macro": false,
      "uses": ["4:5-4:6"]
    }]
}
*/
