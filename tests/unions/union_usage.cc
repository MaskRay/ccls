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
  "types": [{
      "id": 0,
      "usr": "c:@U@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-4:2",
      "vars": [0, 1],
      "instantiations": [2],
      "uses": ["*1:7-1:10", "*6:1-6:4", "*8:10-8:13"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@act#*$@U@Foo#",
      "short_name": "act",
      "qualified_name": "act",
      "definition_spelling": "8:6-8:9",
      "definition_extent": "8:1-10:2"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@U@Foo@FI@a",
      "short_name": "a",
      "qualified_name": "Foo::a",
      "definition_spelling": "2:7-2:8",
      "definition_extent": "2:3-2:12",
      "declaring_type": 0,
      "uses": ["2:7-2:8", "9:5-9:6"]
    }, {
      "id": 1,
      "usr": "c:@U@Foo@FI@b",
      "short_name": "b",
      "qualified_name": "Foo::b",
      "definition_spelling": "3:8-3:9",
      "definition_extent": "3:3-3:13",
      "declaring_type": 0,
      "uses": ["3:8-3:9"]
    }, {
      "id": 2,
      "usr": "c:@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition_spelling": "6:5-6:6",
      "definition_extent": "6:1-6:6",
      "variable_type": 0,
      "uses": ["6:5-6:6", "9:3-9:4"]
    }]
}
*/
