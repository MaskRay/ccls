struct Foo;

void foo(Foo* p0, Foo* p1) {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "instances": [0, 1],
      "uses": ["1:8-1:11", "3:10-3:13", "3:19-3:22"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@Foo#S0_#",
      "short_name": "foo",
      "detailed_name": "void foo(Foo *, Foo *)",
      "parameter_type_descriptions": ["Foo *", "Foo *"],
      "definition_spelling": "3:6-3:9",
      "definition_extent": "3:1-3:30"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:function_param.cc@24@F@foo#*$@S@Foo#S0_#@p0",
      "short_name": "p0",
      "detailed_name": "Foo * p0",
      "definition_spelling": "3:15-3:17",
      "definition_extent": "3:10-3:17",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "uses": ["3:15-3:17"]
    }, {
      "id": 1,
      "usr": "c:function_param.cc@33@F@foo#*$@S@Foo#S0_#@p1",
      "short_name": "p1",
      "detailed_name": "Foo * p1",
      "definition_spelling": "3:24-3:26",
      "definition_extent": "3:19-3:26",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "uses": ["3:24-3:26"]
    }]
}
*/
