class Foo {
  static int member;
};
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
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-3:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:7-1:10"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@member",
      "short_name": "member",
      "detailed_name": "int Foo::member",
      "declaration": "2:14-2:20",
      "is_local": false,
      "is_macro": false,
      "is_global": false,
      "is_member": true,
      "uses": ["2:14-2:20"]
    }]
}
*/
