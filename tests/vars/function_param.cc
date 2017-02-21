struct Foo;

void foo(Foo* p0, Foo* p1) {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "all_uses": ["*1:1:8", "*1:3:10", "*1:3:19"],
      "interesting_uses": ["*1:3:10", "*1:3:19"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@Foo#S0_#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "*1:3:6",
      "all_uses": ["*1:3:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_param.cc@24@F@foo#*$@S@Foo#S0_#@p0",
      "short_name": "p0",
      "qualified_name": "p0",
      "definition": "*1:3:15",
      "variable_type": 0,
      "all_uses": ["*1:3:15"]
    }, {
      "id": 1,
      "usr": "c:function_param.cc@33@F@foo#*$@S@Foo#S0_#@p1",
      "short_name": "p1",
      "qualified_name": "p1",
      "definition": "*1:3:24",
      "variable_type": 0,
      "all_uses": ["*1:3:24"]
    }]
}
*/