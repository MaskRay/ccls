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
      "uses": ["1:8", "*3:1", "*4:1", "*5:1", "*8:3", "*12:1", "*15:14", "*17:8", "*18:8"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "7:7",
      "funcs": [1, 2],
      "uses": ["*7:7", "12:7", "13:6"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declarations": ["3:7", "4:7"],
      "definition": "5:7",
      "uses": ["3:7", "4:7", "5:7"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@F@Get#I#",
      "short_name": "Get",
      "qualified_name": "Foo::Get",
      "declarations": ["8:9"],
      "definition": "12:12",
      "declaring_type": 1,
      "uses": ["8:9", "12:12"]
    }, {
      "id": 2,
      "usr": "c:@S@Foo@F@Empty#",
      "short_name": "Empty",
      "qualified_name": "Foo::Empty",
      "declarations": ["9:8"],
      "definition": "13:11",
      "declaring_type": 1,
      "uses": ["9:8", "13:11"]
    }, {
      "id": 3,
      "usr": "c:@F@external#",
      "short_name": "external",
      "qualified_name": "external",
      "declarations": ["15:20"],
      "uses": ["15:20"]
    }, {
      "id": 4,
      "usr": "c:type_usage_on_return_type.cc@F@bar#",
      "short_name": "bar",
      "qualified_name": "bar",
      "declarations": ["17:14"],
      "definition": "18:14",
      "uses": ["17:14", "18:14"]
    }]
}
*/
