struct Foo;
void foo(Foo* f);

void foo(Foo* f) {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "declaration": "tests/usage/type_usage_on_prototype_parameter.cc:1:8",
      "uses": ["tests/usage/type_usage_on_prototype_parameter.cc:2:15", "tests/usage/type_usage_on_prototype_parameter.cc:4:15"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@Foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "tests/usage/type_usage_on_prototype_parameter.cc:2:6",
      "definition": "tests/usage/type_usage_on_prototype_parameter.cc:4:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_on_prototype_parameter.cc@43@F@foo#*$@S@Foo#@f",
      "short_name": "f",
      "qualified_name": "f",
      "declaration": "tests/usage/type_usage_on_prototype_parameter.cc:4:15",
      "initializations": ["tests/usage/type_usage_on_prototype_parameter.cc:4:15"],
      "variable_type": 0
    }]
}
*/