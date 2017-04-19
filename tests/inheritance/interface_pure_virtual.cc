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
      "detailed_name": "IFoo",
      "definition_spelling": "1:7-1:11",
      "definition_extent": "1:1-3:2",
      "funcs": [0],
      "uses": ["1:7-1:11"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@IFoo@F@foo#",
      "short_name": "foo",
      "detailed_name": "void IFoo::foo()",
      "definition_spelling": "2:16-2:19",
      "definition_extent": "2:3-2:28",
      "declaring_type": 0
    }]
}
*/
