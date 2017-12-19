void called() {}

struct Foo {
  Foo();
};

Foo::Foo() {
  called();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "hover": "Foo",
      "definition_spelling": "3:8-3:11",
      "definition_extent": "3:1-5:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [1],
      "vars": [],
      "instances": [],
      "uses": ["3:8-3:11", "4:3-4:6", "7:6-7:9", "7:1-7:4"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@called#",
      "short_name": "called",
      "detailed_name": "void called()",
      "hover": "void called()",
      "declarations": [],
      "definition_spelling": "1:6-1:12",
      "definition_extent": "1:1-1:17",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": ["1@8:3-8:9"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@S@Foo@F@Foo#",
      "short_name": "Foo",
      "detailed_name": "void Foo::Foo()",
      "hover": "void Foo::Foo()",
      "declarations": [{
          "spelling": "4:3-4:6",
          "extent": "4:3-4:8",
          "content": "Foo()",
          "param_spellings": []
        }],
      "definition_spelling": "7:6-7:9",
      "definition_extent": "7:1-9:2",
      "declaring_type": 0,
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["0@8:3-8:9"]
    }],
  "vars": []
}
*/
