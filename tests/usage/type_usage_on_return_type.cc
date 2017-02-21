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
      "uses": ["1:1:8", "*1:3:1", "*1:4:1", "*1:5:1", "*1:8:3", "*1:12:1", "*1:15:14", "*1:17:8", "*1:18:8"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:7:7",
      "funcs": [1, 2],
      "uses": ["1:7:7", "1:12:7", "1:13:6"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "1:4:7",
      "definition": "1:5:7",
      "uses": ["1:3:7", "1:4:7", "1:5:7"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@F@Get#I#",
      "short_name": "Get",
      "qualified_name": "Foo::Get",
      "declaration": "1:8:9",
      "definition": "1:12:12",
      "declaring_type": 1,
      "uses": ["1:8:9", "1:12:12"]
    }, {
      "id": 2,
      "usr": "c:@S@Foo@F@Empty#",
      "short_name": "Empty",
      "qualified_name": "Foo::Empty",
      "declaration": "1:9:8",
      "definition": "1:13:11",
      "declaring_type": 1,
      "uses": ["1:9:8", "1:13:11"]
    }, {
      "id": 3,
      "usr": "c:@F@external#",
      "short_name": "external",
      "qualified_name": "external",
      "declaration": "1:15:20",
      "uses": ["1:15:20"]
    }, {
      "id": 4,
      "usr": "c:type_usage_on_return_type.cc@F@bar#",
      "short_name": "bar",
      "qualified_name": "bar",
      "declaration": "1:17:14",
      "definition": "1:18:14",
      "uses": ["1:17:14", "1:18:14"]
    }],
  "variables": []
}
*/