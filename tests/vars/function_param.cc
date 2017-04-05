struct Foo;

void foo(Foo* p0, Foo* p1) {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "instantiations": [0, 1],
      "uses": ["1:8", "*3:10", "*3:19"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@Foo#S0_#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "3:6",
      "uses": ["3:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:function_param.cc@24@F@foo#*$@S@Foo#S0_#@p0",
      "short_name": "p0",
      "qualified_name": "p0",
      "definition": "3:15",
      "variable_type": 0,
      "uses": ["3:15"]
    }, {
      "id": 1,
      "usr": "c:function_param.cc@33@F@foo#*$@S@Foo#S0_#@p1",
      "short_name": "p1",
      "qualified_name": "p1",
      "definition": "3:24",
      "variable_type": 0,
      "uses": ["3:24"]
    }]
}
*/
