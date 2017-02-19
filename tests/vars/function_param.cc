struct Foo;

void foo(Foo* p0, Foo* p1) {}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "declaration": "tests/vars/function_param.cc:1:8",
      "uses": ["tests/vars/function_param.cc:3:15", "tests/vars/function_param.cc:3:24"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@Foo#S0_#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_param.cc:3:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_param.cc@24@F@foo#*$@S@Foo#S0_#@p0",
      "short_name": "p0",
      "qualified_name": "p0",
      "declaration": "tests/vars/function_param.cc:3:15",
      "initializations": ["tests/vars/function_param.cc:3:15"],
      "variable_type": 0
    }, {
      "id": 1,
      "usr": "c:function_param.cc@33@F@foo#*$@S@Foo#S0_#@p1",
      "short_name": "p1",
      "qualified_name": "p1",
      "declaration": "tests/vars/function_param.cc:3:24",
      "initializations": ["tests/vars/function_param.cc:3:24"],
      "variable_type": 0
    }]
}
*/