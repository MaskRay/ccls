struct Foo;

void foo(Foo* f, Foo*);
void foo(Foo* f, Foo*) {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "declaration": "tests/usage/type_usage_declare_param_prototype.cc:1:8",
      "uses": ["tests/usage/type_usage_declare_param_prototype.cc:3:15", "tests/usage/type_usage_declare_param_prototype.cc:3:22", "tests/usage/type_usage_declare_param_prototype.cc:4:15", "tests/usage/type_usage_declare_param_prototype.cc:4:22"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@Foo#S0_#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "tests/usage/type_usage_declare_param_prototype.cc:3:6",
      "definition": "tests/usage/type_usage_declare_param_prototype.cc:4:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_declare_param_prototype.cc@49@F@foo#*$@S@Foo#S0_#@f",
      "short_name": "f",
      "qualified_name": "f",
      "declaration": "tests/usage/type_usage_declare_param_prototype.cc:4:15",
      "initializations": ["tests/usage/type_usage_declare_param_prototype.cc:4:15"],
      "variable_type": 0
    }]
}
*/