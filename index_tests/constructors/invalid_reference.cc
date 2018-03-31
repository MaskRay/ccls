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
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 23,
      "declarations": ["4:6-4:9|-1|1|4"],
      "spell": "1:8-1:11|-1|1|2",
      "extent": "1:1-1:14|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["4:6-4:9|-1|1|4", "4:1-4:4|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 17319723337446061757,
      "detailed_name": "void Foo::Foo()",
      "short_name": "Foo",
      "kind": 9,
      "storage": 1,
      "declarations": [],
      "spell": "4:6-4:9|0|2|2",
      "extent": "4:1-4:11|-1|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/
