struct ForwardType;
struct ImplementedType {};

void Foo() {
  ForwardType* a;
  ImplementedType b;
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "instances": [0],
      "uses": ["1:8-1:19", "5:3-5:14"]
    }, {
      "id": 1,
      "usr": "c:@S@ImplementedType",
      "short_name": "ImplementedType",
      "detailed_name": "ImplementedType",
      "definition_spelling": "2:8-2:23",
      "definition_extent": "2:1-2:26",
      "instances": [1],
      "uses": ["2:8-2:23", "6:3-6:18"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@Foo#",
      "short_name": "Foo",
      "detailed_name": "void Foo()",
      "definition_spelling": "4:6-4:9",
      "definition_extent": "4:1-7:2"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_declare_local.cc@67@F@Foo#@a",
      "short_name": "a",
      "detailed_name": "ForwardType * a",
      "definition_spelling": "5:16-5:17",
      "definition_extent": "5:3-5:17",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "uses": ["5:16-5:17"]
    }, {
      "id": 1,
      "usr": "c:type_usage_declare_local.cc@86@F@Foo#@b",
      "short_name": "b",
      "detailed_name": "ImplementedType b",
      "definition_spelling": "6:19-6:20",
      "definition_extent": "6:3-6:20",
      "variable_type": 1,
      "is_local": true,
      "is_macro": false,
      "uses": ["6:19-6:20"]
    }]
}
*/
