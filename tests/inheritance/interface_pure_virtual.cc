class IFoo {
  virtual void foo() = 0;
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@IFoo",
      "short_name": "IFoo",
      "detailed_name": "IFoo",
      "definition_spelling": "1:7-1:11",
      "definition_extent": "1:1-3:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["1:7-1:11"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@IFoo@F@foo#",
      "short_name": "foo",
      "detailed_name": "void IFoo::foo()",
      "hover": "void IFoo::foo()",
      "declarations": [{
          "spelling": "2:16-2:19",
          "extent": "2:3-2:25",
          "content": "virtual void foo() = 0",
          "param_spellings": []
        }],
      "declaring_type": 0,
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
*/
