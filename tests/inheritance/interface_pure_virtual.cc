class IFoo {
  virtual void foo() = 0 {}
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@IFoo",
      "short_name": "IFoo",
      "qualified_name": "IFoo",
      "definition": "tests/inheritance/interface_pure_virtual.cc:1:7",
      "funcs": [0],
      "all_uses": ["tests/inheritance/interface_pure_virtual.cc:1:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@IFoo@F@foo#",
      "short_name": "foo",
      "qualified_name": "IFoo::foo",
      "definition": "tests/inheritance/interface_pure_virtual.cc:2:16",
      "declaring_type": 0,
      "all_uses": ["tests/inheritance/interface_pure_virtual.cc:2:16"]
    }],
  "variables": []
}
*/