struct ForwardType;
struct ImplementedType {};

struct Foo {
  ForwardType* a;
  ImplementedType b;
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "short_name": "",
      "detailed_name": "",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["1:8-1:19", "5:3-5:14"]
    }, {
      "id": 1,
      "usr": "c:@S@ImplementedType",
      "short_name": "ImplementedType",
      "detailed_name": "ImplementedType",
      "definition_spelling": "2:8-2:23",
      "definition_extent": "2:1-2:26",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [1],
      "uses": ["2:8-2:23", "6:3-6:18"]
    }, {
      "id": 2,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "4:8-4:11",
      "definition_extent": "4:1-7:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1],
      "instances": [],
      "uses": ["4:8-4:11"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@a",
      "short_name": "a",
      "detailed_name": "ForwardType * Foo::a",
      "definition_spelling": "5:16-5:17",
      "definition_extent": "5:3-5:17",
      "variable_type": 0,
      "declaring_type": 2,
      "is_local": false,
      "is_macro": false,
      "uses": ["5:16-5:17"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@b",
      "short_name": "b",
      "detailed_name": "ImplementedType Foo::b",
      "definition_spelling": "6:19-6:20",
      "definition_extent": "6:3-6:20",
      "variable_type": 1,
      "declaring_type": 2,
      "is_local": false,
      "is_macro": false,
      "uses": ["6:19-6:20"]
    }]
}
*/
