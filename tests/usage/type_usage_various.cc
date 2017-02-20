class Foo {
  Foo* make();
};

Foo* Foo::make() { 
  Foo f;
  return nullptr;
}

extern Foo foo;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/type_usage_various.cc:1:7",
      "funcs": [0],
      "all_uses": ["tests/usage/type_usage_various.cc:1:7", "tests/usage/type_usage_various.cc:2:3", "tests/usage/type_usage_various.cc:5:1", "tests/usage/type_usage_various.cc:5:6", "tests/usage/type_usage_various.cc:6:3", "tests/usage/type_usage_various.cc:10:8"],
      "interesting_uses": ["tests/usage/type_usage_various.cc:2:3", "tests/usage/type_usage_various.cc:5:1", "tests/usage/type_usage_various.cc:6:3", "tests/usage/type_usage_various.cc:10:8"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@make#",
      "short_name": "make",
      "qualified_name": "Foo::make",
      "definition": "tests/usage/type_usage_various.cc:5:11",
      "declaring_type": 0,
      "all_uses": ["tests/usage/type_usage_various.cc:2:8", "tests/usage/type_usage_various.cc:5:11"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_various.cc@58@S@Foo@F@make#@f",
      "short_name": "f",
      "qualified_name": "f",
      "declaration": "tests/usage/type_usage_various.cc:6:7",
      "variable_type": 0,
      "all_uses": ["tests/usage/type_usage_various.cc:6:7"]
    }, {
      "id": 1,
      "usr": "c:@foo",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "tests/usage/type_usage_various.cc:10:12",
      "variable_type": 0,
      "all_uses": ["tests/usage/type_usage_various.cc:10:12"]
    }]
}
*/