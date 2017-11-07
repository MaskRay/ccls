struct Foo {
  static int x;
};

void accept(int);

void foo() {
  accept(Foo::x);
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:8-1:11",
      "definition_extent": "1:1-3:2",
      "uses": ["1:8-1:11", "8:10-8:13"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@accept#I#",
      "short_name": "accept",
      "detailed_name": "void accept(int)",
      "is_constructor": false,
      "parameter_type_descriptions": ["int"],
      "declarations": [{
          "spelling": "5:6-5:12",
          "extent": "5:1-5:17",
          "content": "void accept(int)",
          "param_spellings": ["5:16-5:16"]
        }],
      "callers": ["1@8:3-8:9"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "is_constructor": false,
      "definition_spelling": "7:6-7:9",
      "definition_extent": "7:1-9:2",
      "callees": ["0@8:3-8:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@x",
      "short_name": "x",
      "detailed_name": "int Foo::x",
      "declaration": "2:14-2:15",
      "is_local": false,
      "is_macro": false,
      "uses": ["2:14-2:15", "8:15-8:16"]
    }]
}
*/
