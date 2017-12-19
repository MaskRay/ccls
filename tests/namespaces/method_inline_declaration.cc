namespace hello {
class Foo {
  void foo() {}
};
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo",
      "short_name": "Foo",
      "detailed_name": "hello::Foo",
      "hover": "hello::Foo",
      "definition_spelling": "2:7-2:10",
      "definition_extent": "2:1-4:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["2:7-2:10"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@N@hello@S@Foo@F@foo#",
      "short_name": "foo",
      "detailed_name": "void hello::Foo::foo()",
      "hover": "void hello::Foo::foo()",
      "declarations": [],
      "definition_spelling": "3:8-3:11",
      "definition_extent": "3:3-3:16",
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
