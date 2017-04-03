struct Foo;

void foo(Foo* f, Foo*);
void foo(Foo* f, Foo*) {}

/*
// TODO: No interesting usage on prototype. But maybe that's ok!
// TODO: We should have the same variable declared for both prototype and
//       declaration. So it should have a usage marker on both. Then we could
//       rename parameters!

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "instantiations": [0],
      "uses": ["1:1:8", "1:3:10", "1:3:18", "*1:4:10", "*1:4:18"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@Foo#S0_#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declarations": ["1:3:6"],
      "definition": "1:4:6",
      "uses": ["1:3:6", "1:4:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_declare_param_prototype.cc@49@F@foo#*$@S@Foo#S0_#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "1:4:15",
      "variable_type": 0,
      "uses": ["1:4:15"]
    }]
}
*/
