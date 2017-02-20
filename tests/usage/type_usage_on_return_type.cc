struct Type;

Type* foo();
Type* foo();
Type* foo() {}

class Foo {
  Type* Get(int);
  void Empty();
};

Type* Foo::Get(int) {}
void Foo::Empty() {}

// TODO: Add static
// TODO: Add extern?
// TODO: verify interesting usage is reported

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Type",
      "short_name": "Type",
      "qualified_name": "Type",
      "declaration": "tests/usage/type_usage_on_return_type.cc:1:8",
      "uses": ["tests/usage/type_usage_on_return_type.cc:3:1", "tests/usage/type_usage_on_return_type.cc:4:1", "tests/usage/type_usage_on_return_type.cc:5:1", "tests/usage/type_usage_on_return_type.cc:8:3", "tests/usage/type_usage_on_return_type.cc:12:1"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/type_usage_on_return_type.cc:7:7",
      "funcs": [1, 2]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "tests/usage/type_usage_on_return_type.cc:4:7",
      "definition": "tests/usage/type_usage_on_return_type.cc:5:7"
    }, {
      "id": 1,
      "usr": "c:@S@Foo@F@Get#I#",
      "short_name": "Get",
      "qualified_name": "Foo::Get",
      "declaration": "tests/usage/type_usage_on_return_type.cc:8:9",
      "definition": "tests/usage/type_usage_on_return_type.cc:12:12",
      "declaring_type": 1
    }, {
      "id": 2,
      "usr": "c:@S@Foo@F@Empty#",
      "short_name": "Empty",
      "qualified_name": "Foo::Empty",
      "declaration": "tests/usage/type_usage_on_return_type.cc:9:8",
      "definition": "tests/usage/type_usage_on_return_type.cc:13:11",
      "declaring_type": 1
    }],
  "variables": []
}
*/