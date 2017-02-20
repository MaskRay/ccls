struct Foo;
using Foo1 = Foo*;
typedef Foo Foo2;
using Foo3 = Foo1;
using Foo4 = int;

void accept(Foo*) {}
void accept1(Foo1*) {}
void accept2(Foo2*) {}
void accept3(Foo3*) {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "all_uses": ["tests/usage/type_usage_typedef_and_using.cc:1:8", "tests/usage/type_usage_typedef_and_using.cc:2:14", "tests/usage/type_usage_typedef_and_using.cc:3:9", "tests/usage/type_usage_typedef_and_using.cc:7:13"],
      "interesting_uses": ["tests/usage/type_usage_typedef_and_using.cc:2:14", "tests/usage/type_usage_typedef_and_using.cc:3:9", "tests/usage/type_usage_typedef_and_using.cc:7:13"]
    }, {
      "id": 1,
      "usr": "c:@Foo1",
      "short_name": "Foo1",
      "qualified_name": "Foo1",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:2:7",
      "alias_of": 0,
      "all_uses": ["tests/usage/type_usage_typedef_and_using.cc:2:7", "tests/usage/type_usage_typedef_and_using.cc:4:14", "tests/usage/type_usage_typedef_and_using.cc:8:14"],
      "interesting_uses": ["tests/usage/type_usage_typedef_and_using.cc:4:14", "tests/usage/type_usage_typedef_and_using.cc:8:14"]
    }, {
      "id": 2,
      "usr": "c:type_usage_typedef_and_using.cc@T@Foo2",
      "short_name": "Foo2",
      "qualified_name": "Foo2",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:3:13",
      "alias_of": 0,
      "all_uses": ["tests/usage/type_usage_typedef_and_using.cc:3:13", "tests/usage/type_usage_typedef_and_using.cc:9:14"],
      "interesting_uses": ["tests/usage/type_usage_typedef_and_using.cc:9:14"]
    }, {
      "id": 3,
      "usr": "c:@Foo3",
      "short_name": "Foo3",
      "qualified_name": "Foo3",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:4:7",
      "alias_of": 1,
      "all_uses": ["tests/usage/type_usage_typedef_and_using.cc:4:7", "tests/usage/type_usage_typedef_and_using.cc:10:14"],
      "interesting_uses": ["tests/usage/type_usage_typedef_and_using.cc:10:14"]
    }, {
      "id": 4,
      "usr": "c:@Foo4",
      "short_name": "Foo4",
      "qualified_name": "Foo4",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:5:7",
      "all_uses": ["tests/usage/type_usage_typedef_and_using.cc:5:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@accept#*$@S@Foo#",
      "short_name": "accept",
      "qualified_name": "accept",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:7:6",
      "all_uses": ["tests/usage/type_usage_typedef_and_using.cc:7:6"]
    }, {
      "id": 1,
      "usr": "c:@F@accept1#**$@S@Foo#",
      "short_name": "accept1",
      "qualified_name": "accept1",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:8:6",
      "all_uses": ["tests/usage/type_usage_typedef_and_using.cc:8:6"]
    }, {
      "id": 2,
      "usr": "c:@F@accept2#*$@S@Foo#",
      "short_name": "accept2",
      "qualified_name": "accept2",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:9:6",
      "all_uses": ["tests/usage/type_usage_typedef_and_using.cc:9:6"]
    }, {
      "id": 3,
      "usr": "c:@F@accept3#**$@S@Foo#",
      "short_name": "accept3",
      "qualified_name": "accept3",
      "definition": "tests/usage/type_usage_typedef_and_using.cc:10:6",
      "all_uses": ["tests/usage/type_usage_typedef_and_using.cc:10:6"]
    }],
  "variables": []
}
*/