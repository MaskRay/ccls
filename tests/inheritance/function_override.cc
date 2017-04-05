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
      "definition": "1:7",
      "derived": [1],
      "funcs": [0],
      "uses": ["*1:7", "*4:24"]
    }, {
      "id": 1,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "qualified_name": "Derived",
      "definition": "4:7",
      "parents": [0],
      "funcs": [1],
      "uses": ["*4:7"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Root@F@foo#",
      "short_name": "foo",
      "qualified_name": "Root::foo",
      "declarations": ["2:16"],
      "declaring_type": 0,
      "derived": [1],
      "uses": ["2:16"]
    }, {
      "id": 1,
      "usr": "c:@S@Derived@F@foo#",
      "short_name": "foo",
      "qualified_name": "Derived::foo",
      "definition": "5:8",
      "declaring_type": 1,
      "base": 0,
      "uses": ["5:8"]
    }]
}
*/
