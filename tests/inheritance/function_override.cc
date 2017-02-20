class Root {
  virtual void foo();
};
class Derived : public Root {
  void foo() override {}
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Root",
      "short_name": "Root",
      "qualified_name": "Root",
      "definition": "tests/inheritance/function_override.cc:1:7",
      "derived": [1],
      "all_uses": ["tests/inheritance/function_override.cc:1:7", "tests/inheritance/function_override.cc:4:24"]
    }, {
      "id": 1,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "qualified_name": "Derived",
      "definition": "tests/inheritance/function_override.cc:4:7",
      "parents": [0],
      "funcs": [1],
      "all_uses": ["tests/inheritance/function_override.cc:4:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Root@F@foo#",
      "short_name": "foo",
      "qualified_name": "Root::foo",
      "declaration": "tests/inheritance/function_override.cc:2:16",
      "derived": [1],
      "all_uses": ["tests/inheritance/function_override.cc:2:16"]
    }, {
      "id": 1,
      "usr": "c:@S@Derived@F@foo#",
      "short_name": "foo",
      "qualified_name": "Derived::foo",
      "definition": "tests/inheritance/function_override.cc:5:8",
      "declaring_type": 1,
      "base": 0,
      "all_uses": ["tests/inheritance/function_override.cc:5:8"]
    }],
  "variables": []
}
*/