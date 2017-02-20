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
      "all_uses": ["tests/usage/type_usage_declare_param_prototype.cc:1:8", "tests/usage/type_usage_declare_param_prototype.cc:3:10", "tests/usage/type_usage_declare_param_prototype.cc:3:18", "tests/usage/type_usage_declare_param_prototype.cc:4:10", "tests/usage/type_usage_declare_param_prototype.cc:4:18"],
      "interesting_uses": ["tests/usage/type_usage_declare_param_prototype.cc:4:10"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@Foo#S0_#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/type_usage_declare_param_prototype.cc:4:6",
      "all_uses": ["tests/usage/type_usage_declare_param_prototype.cc:3:6", "tests/usage/type_usage_declare_param_prototype.cc:4:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_declare_param_prototype.cc@49@F@foo#*$@S@Foo#S0_#@f",
      "short_name": "f",
      "qualified_name": "f",
      "declaration": "tests/usage/type_usage_declare_param_prototype.cc:4:15",
      "variable_type": 0,
      "all_uses": ["tests/usage/type_usage_declare_param_prototype.cc:4:15"]
    }]
}
*/