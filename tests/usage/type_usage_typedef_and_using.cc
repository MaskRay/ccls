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

// TODO: I think we should mark using Foo1 = foo as an interesting usage on
//       Foo1.
// TODO: Also mark class Foo {} as an interesting usage of Foo.

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "all_uses": ["1:1:8", "*1:2:14", "*1:3:9", "*1:7:13"]
    }, {
      "id": 1,
      "usr": "c:@Foo1",
      "short_name": "Foo1",
      "qualified_name": "Foo1",
      "definition": "1:2:7",
      "alias_of": 0,
      "all_uses": ["1:2:7", "*1:4:14", "*1:8:14"]
    }, {
      "id": 2,
      "usr": "c:type_usage_typedef_and_using.cc@T@Foo2",
      "short_name": "Foo2",
      "qualified_name": "Foo2",
      "definition": "1:3:13",
      "alias_of": 0,
      "all_uses": ["1:3:13", "*1:9:14"]
    }, {
      "id": 3,
      "usr": "c:@Foo3",
      "short_name": "Foo3",
      "qualified_name": "Foo3",
      "definition": "1:4:7",
      "alias_of": 1,
      "all_uses": ["1:4:7", "*1:10:14"]
    }, {
      "id": 4,
      "usr": "c:@Foo4",
      "short_name": "Foo4",
      "qualified_name": "Foo4",
      "definition": "1:5:7",
      "all_uses": ["1:5:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@accept#*$@S@Foo#",
      "short_name": "accept",
      "qualified_name": "accept",
      "definition": "1:7:6",
      "all_uses": ["1:7:6"]
    }, {
      "id": 1,
      "usr": "c:@F@accept1#**$@S@Foo#",
      "short_name": "accept1",
      "qualified_name": "accept1",
      "definition": "1:8:6",
      "all_uses": ["1:8:6"]
    }, {
      "id": 2,
      "usr": "c:@F@accept2#*$@S@Foo#",
      "short_name": "accept2",
      "qualified_name": "accept2",
      "definition": "1:9:6",
      "all_uses": ["1:9:6"]
    }, {
      "id": 3,
      "usr": "c:@F@accept3#**$@S@Foo#",
      "short_name": "accept3",
      "qualified_name": "accept3",
      "definition": "1:10:6",
      "all_uses": ["1:10:6"]
    }],
  "variables": []
}
*/