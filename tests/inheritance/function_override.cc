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
      "definition": "1:1:7",
      "derived": [1],
      "all_uses": ["1:1:7", "*1:4:24"]
    }, {
      "id": 1,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "qualified_name": "Derived",
      "definition": "1:4:7",
      "parents": [0],
      "funcs": [1],
      "all_uses": ["1:4:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Root@F@foo#",
      "short_name": "foo",
      "qualified_name": "Root::foo",
      "declaration": "1:2:16",
      "derived": [1],
      "all_uses": ["1:2:16"]
    }, {
      "id": 1,
      "usr": "c:@S@Derived@F@foo#",
      "short_name": "foo",
      "qualified_name": "Derived::foo",
      "definition": "1:5:8",
      "declaring_type": 1,
      "base": 0,
      "all_uses": ["1:5:8"]
    }],
  "variables": []
}
*/