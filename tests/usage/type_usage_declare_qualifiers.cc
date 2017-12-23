struct Type {};

void foo(Type& a0, const Type& a1) {
  Type a2;
  Type* a3;
  const Type* a4;
  const Type* const a5 = nullptr;
}
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Type",
      "short_name": "Type",
      "detailed_name": "Type",
      "definition_spelling": "1:8-1:12",
      "definition_extent": "1:1-1:15",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1, 2, 3, 4, 5],
      "uses": ["1:8-1:12", "3:10-3:14", "3:26-3:30", "4:3-4:7", "5:3-5:7", "6:9-6:13", "7:9-7:13"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@foo#&$@S@Type#&1S1_#",
      "short_name": "foo",
      "detailed_name": "void foo(Type &, const Type &)",
      "hover": "void foo(Type &, const Type &)",
      "declarations": [],
      "definition_spelling": "3:6-3:9",
      "definition_extent": "3:1-8:2",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_declare_qualifiers.cc@28@F@foo#&$@S@Type#&1S1_#@a0",
      "short_name": "a0",
      "detailed_name": "Type & a0",
      "definition_spelling": "3:16-3:18",
      "definition_extent": "3:10-3:18",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["3:16-3:18"]
    }, {
      "id": 1,
      "usr": "c:type_usage_declare_qualifiers.cc@38@F@foo#&$@S@Type#&1S1_#@a1",
      "short_name": "a1",
      "detailed_name": "const Type & a1",
      "definition_spelling": "3:32-3:34",
      "definition_extent": "3:20-3:34",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["3:32-3:34"]
    }, {
      "id": 2,
      "usr": "c:type_usage_declare_qualifiers.cc@59@F@foo#&$@S@Type#&1S1_#@a2",
      "short_name": "a2",
      "detailed_name": "Type a2",
      "definition_spelling": "4:8-4:10",
      "definition_extent": "4:3-4:10",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["4:8-4:10"]
    }, {
      "id": 3,
      "usr": "c:type_usage_declare_qualifiers.cc@71@F@foo#&$@S@Type#&1S1_#@a3",
      "short_name": "a3",
      "detailed_name": "Type * a3",
      "definition_spelling": "5:9-5:11",
      "definition_extent": "5:3-5:11",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["5:9-5:11"]
    }, {
      "id": 4,
      "usr": "c:type_usage_declare_qualifiers.cc@84@F@foo#&$@S@Type#&1S1_#@a4",
      "short_name": "a4",
      "detailed_name": "const Type * a4",
      "definition_spelling": "6:15-6:17",
      "definition_extent": "6:3-6:17",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["6:15-6:17"]
    }, {
      "id": 5,
      "usr": "c:type_usage_declare_qualifiers.cc@103@F@foo#&$@S@Type#&1S1_#@a5",
      "short_name": "a5",
      "detailed_name": "const Type *const a5",
      "definition_spelling": "7:21-7:23",
      "definition_extent": "7:3-7:33",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["7:21-7:23"]
    }]
}
*/
