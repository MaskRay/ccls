struct Foo;
using Foo1 = Foo;
typedef Foo Foo2;
using Foo3 = Foo1;

void accept(Foo*);
void accept1(Foo1*);
void accept2(Foo2*);
void accept3(Foo3*);

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "declaration": "tests/usage/type_usage_typedef_and_using.cc:1:8",
      "uses": ["tests/usage/type_usage_typedef_and_using.cc:6:17"]
    }, {
      "id": 1,
      "usr": "c:@Foo1",
      "short_name": "Foo1",
      "qualified_name": "Foo1",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:2:7",
      "alias_of": 0,
      "uses": ["tests/usage/type_usage_typedef_and_using.cc:7:19"]
    }, {
      "id": 2,
      "usr": "c:type_usage_typedef_and_using.cc@T@Foo2",
      "short_name": "Foo2",
      "qualified_name": "Foo2",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:3:13",
      "alias_of": 0,
      "uses": ["tests/usage/type_usage_typedef_and_using.cc:8:19"]
    }, {
      "id": 3,
      "usr": "c:@Foo3",
      "short_name": "Foo3",
      "qualified_name": "Foo3",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:4:7",
      "alias_of": 1,
      "uses": ["tests/usage/type_usage_typedef_and_using.cc:9:19"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@accept#*$@S@Foo#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declaration": "tests/usage/type_usage_typedef_and_using.cc:6:6"
    }, {
      "id": 1,
      "usr": "c:@F@accept1#*$@S@Foo#",
      "short_name": "accept1",
      "qualified_name": "accept1",
      "declaration": "tests/usage/type_usage_typedef_and_using.cc:7:6"
    }, {
      "id": 2,
      "usr": "c:@F@accept2#*$@S@Foo#",
      "short_name": "accept2",
      "qualified_name": "accept2",
      "declaration": "tests/usage/type_usage_typedef_and_using.cc:8:6"
    }, {
      "id": 3,
      "usr": "c:@F@accept3#*$@S@Foo#",
      "short_name": "accept3",
      "qualified_name": "accept3",
      "declaration": "tests/usage/type_usage_typedef_and_using.cc:9:6"
    }],
  "variables": []
}
*/