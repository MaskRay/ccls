union Foo {
  int a : 5;
  bool b : 3;
};

Foo f;

void act(Foo*) {
  f.a = 3;
}

/*
// TODO: instantiations on Foo should include parameter?

OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@U@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-4:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1],
      "instances": [2],
      "uses": ["1:7-1:10", "6:1-6:4", "8:10-8:13"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@act#*$@U@Foo#",
      "short_name": "act",
      "detailed_name": "void act(Foo *)",
      "declarations": [],
      "definition_spelling": "8:6-8:9",
      "definition_extent": "8:1-10:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@U@Foo@FI@a",
      "short_name": "a",
      "detailed_name": "int Foo::a",
      "definition_spelling": "2:7-2:8",
      "definition_extent": "2:3-2:12",
      "declaring_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["2:7-2:8", "9:5-9:6"]
    }, {
      "id": 1,
      "usr": "c:@U@Foo@FI@b",
      "short_name": "b",
      "detailed_name": "bool Foo::b",
      "definition_spelling": "3:8-3:9",
      "definition_extent": "3:3-3:13",
      "declaring_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["3:8-3:9"]
    }, {
      "id": 2,
      "usr": "c:@f",
      "short_name": "f",
      "detailed_name": "Foo f",
      "definition_spelling": "6:5-6:6",
      "definition_extent": "6:1-6:6",
      "variable_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["6:5-6:6", "9:3-9:4"]
    }]
}
*/
