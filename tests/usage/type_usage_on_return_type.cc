struct Type;

Type* foo();
Type* foo();
Type* foo() {}

/*
// TODO: We should try to get the right location for type uses so it points to
//       the return type and not the function name.

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Type",
      "short_name": "Type",
      "qualified_name": "Type",
      "declaration": "tests/usage/type_usage_on_return_type.cc:1:8",
      "uses": ["tests/usage/type_usage_on_return_type.cc:3:7", "tests/usage/type_usage_on_return_type.cc:4:7", "tests/usage/type_usage_on_return_type.cc:5:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "tests/usage/type_usage_on_return_type.cc:4:7",
      "definition": "tests/usage/type_usage_on_return_type.cc:5:7"
    }],
  "variables": []
}
*/