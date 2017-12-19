struct Type {
  Type() {}
};

void Make() {
  Type foo;
  auto foo = Type();
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
      "hover": "Type",
      "definition_spelling": "1:8-1:12",
      "definition_extent": "1:1-3:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [0],
      "uses": ["1:8-1:12", "2:3-2:7", "6:3-6:7"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Type@F@Type#",
      "short_name": "Type",
      "detailed_name": "void Type::Type()",
      "hover": "void Type::Type()",
      "declarations": [],
      "definition_spelling": "2:3-2:7",
      "definition_extent": "2:3-2:12",
      "declaring_type": 0,
      "derived": [],
      "locals": [],
      "callers": ["~1@6:8-6:11"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@Make#",
      "short_name": "Make",
      "detailed_name": "void Make()",
      "hover": "void Make()",
      "declarations": [],
      "definition_spelling": "5:6-5:10",
      "definition_extent": "5:1-8:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["~0@6:8-6:11"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:implicit_constructor.cc@51@F@Make#@foo",
      "short_name": "foo",
      "detailed_name": "Type foo",
      "hover": "Type",
      "definition_spelling": "6:8-6:11",
      "definition_extent": "6:3-6:11",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "uses": ["6:8-6:11"]
    }, {
      "id": 1,
      "usr": "c:implicit_constructor.cc@64@F@Make#@foo",
      "short_name": "foo",
      "detailed_name": "auto foo",
      "hover": "auto",
      "definition_spelling": "7:8-7:11",
      "definition_extent": "7:3-7:11",
      "is_local": true,
      "is_macro": false,
      "uses": ["7:8-7:11"]
    }]
}
*/
