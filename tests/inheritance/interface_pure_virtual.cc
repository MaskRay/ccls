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
      "definition": "1:1:7",
      "funcs": [0],
      "uses": ["*1:1:7"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@IFoo@F@foo#",
      "short_name": "foo",
      "qualified_name": "IFoo::foo",
      "definition": "1:2:16",
      "declaring_type": 0,
      "uses": ["1:2:16"]
    }]
}
*/
