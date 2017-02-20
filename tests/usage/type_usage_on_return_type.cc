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

extern const Type& external();

static Type* bar();
static Type* bar() {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Type",
      "all_uses": ["tests/usage/type_usage_on_return_type.cc:1:8", "tests/usage/type_usage_on_return_type.cc:3:1", "tests/usage/type_usage_on_return_type.cc:4:1", "tests/usage/type_usage_on_return_type.cc:5:1", "tests/usage/type_usage_on_return_type.cc:8:3", "tests/usage/type_usage_on_return_type.cc:12:1", "tests/usage/type_usage_on_return_type.cc:15:14", "tests/usage/type_usage_on_return_type.cc:17:8", "tests/usage/type_usage_on_return_type.cc:18:8"],
      "interesting_uses": ["tests/usage/type_usage_on_return_type.cc:3:1", "tests/usage/type_usage_on_return_type.cc:4:1", "tests/usage/type_usage_on_return_type.cc:5:1", "tests/usage/type_usage_on_return_type.cc:8:3", "tests/usage/type_usage_on_return_type.cc:12:1", "tests/usage/type_usage_on_return_type.cc:15:14", "tests/usage/type_usage_on_return_type.cc:17:8", "tests/usage/type_usage_on_return_type.cc:18:8"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/type_usage_on_return_type.cc:7:7",
      "funcs": [1, 2],
      "all_uses": ["tests/usage/type_usage_on_return_type.cc:7:7", "tests/usage/type_usage_on_return_type.cc:12:7", "tests/usage/type_usage_on_return_type.cc:13:6"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "tests/usage/type_usage_on_return_type.cc:4:7",
      "definition": "tests/usage/type_usage_on_return_type.cc:5:7",
      "all_uses": ["tests/usage/type_usage_on_return_type.cc:3:7", "tests/usage/type_usage_on_return_type.cc:4:7", "tests/usage/type_usage_on_return_type.cc:5:7"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@F@Get#I#",
      "short_name": "Get",
      "qualified_name": "Foo::Get",
      "declaration": "tests/usage/type_usage_on_return_type.cc:8:9",
      "definition": "tests/usage/type_usage_on_return_type.cc:12:12",
      "declaring_type": 1,
      "all_uses": ["tests/usage/type_usage_on_return_type.cc:8:9", "tests/usage/type_usage_on_return_type.cc:12:12"]
    }, {
      "id": 2,
      "usr": "c:@S@Foo@F@Empty#",
      "short_name": "Empty",
      "qualified_name": "Foo::Empty",
      "declaration": "tests/usage/type_usage_on_return_type.cc:9:8",
      "definition": "tests/usage/type_usage_on_return_type.cc:13:11",
      "declaring_type": 1,
      "all_uses": ["tests/usage/type_usage_on_return_type.cc:9:8", "tests/usage/type_usage_on_return_type.cc:13:11"]
    }, {
      "id": 3,
      "usr": "c:@F@external#",
      "short_name": "external",
      "qualified_name": "external",
      "declaration": "tests/usage/type_usage_on_return_type.cc:15:20",
      "all_uses": ["tests/usage/type_usage_on_return_type.cc:15:20"]
    }, {
      "id": 4,
      "usr": "c:type_usage_on_return_type.cc@F@bar#",
      "short_name": "bar",
      "qualified_name": "bar",
      "declaration": "tests/usage/type_usage_on_return_type.cc:17:14",
      "definition": "tests/usage/type_usage_on_return_type.cc:18:14",
      "all_uses": ["tests/usage/type_usage_on_return_type.cc:17:14", "tests/usage/type_usage_on_return_type.cc:18:14"]
    }],
  "variables": []
}
*/