template<typename T>
struct Foo {
  static int foo() {
    return 3;
  }
};

int a = Foo<int>::foo();
int b = Foo<bool>::foo();

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "2:8-2:11",
      "definition_extent": "2:1-6:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["2:8-2:11", "8:9-8:12", "9:9-9:12"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@ST>1#T@Foo@F@foo#S",
      "short_name": "foo",
      "detailed_name": "int Foo::foo()",
      "hover": "int Foo::foo()",
      "declarations": [],
      "definition_spelling": "3:14-3:17",
      "definition_extent": "3:3-5:4",
      "declaring_type": 0,
      "base": [],
      "derived": [],
      "locals": [],
      "callers": ["-1@8:19-8:22", "-1@9:20-9:23"],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "detailed_name": "int a",
      "definition_spelling": "8:5-8:6",
      "definition_extent": "8:1-8:24",
      "is_local": false,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["8:5-8:6"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "detailed_name": "int b",
      "definition_spelling": "9:5-9:6",
      "definition_extent": "9:1-9:25",
      "is_local": false,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["9:5-9:6"]
    }]
}
*/
