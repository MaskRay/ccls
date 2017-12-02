class Foo {};
void f() {
  auto x = new Foo();
  auto* y = new Foo();
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-1:13",
      "instances": [0, 1],
      "uses": ["1:7-1:10", "3:16-3:19", "4:17-4:20"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@f#",
      "short_name": "f",
      "detailed_name": "void f()",
      "definition_spelling": "2:6-2:7",
      "definition_extent": "2:1-5:2"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:deduce_auto_type.cc@29@F@f#@x",
      "short_name": "x",
      "detailed_name": "Foo * x",
      "definition_spelling": "3:8-3:9",
      "definition_extent": "3:3-3:21",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "uses": ["3:8-3:9"]
    }, {
      "id": 1,
      "usr": "c:deduce_auto_type.cc@52@F@f#@y",
      "short_name": "y",
      "detailed_name": "Foo * y",
      "definition_spelling": "4:9-4:10",
      "definition_extent": "4:3-4:22",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "uses": ["4:9-4:10"]
    }]
}
*/
