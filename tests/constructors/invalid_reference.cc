struct Foo {};

template<class T>
Foo::Foo() {}

/*
EXTRA_FLAGS:
-fms-compatibility
-fdelayed-template-parsing

OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:8-1:11",
      "definition_extent": "1:1-1:14",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["1:8-1:11", "4:6-4:9", "4:1-4:4"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Foo@FT@>1#TFoo#v#",
      "short_name": "Foo",
      "detailed_name": "void Foo::Foo()",
      "declarations": [],
      "definition_spelling": "4:6-4:9",
      "definition_extent": "4:1-4:11",
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
