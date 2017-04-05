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
      "uses": ["1:8", "*2:14", "*3:9", "*7:13"]
    }, {
      "id": 1,
      "usr": "c:@Foo1",
      "short_name": "Foo1",
      "qualified_name": "Foo1",
      "definition": "2:7",
      "alias_of": 0,
      "uses": ["*2:7", "*4:14", "*8:14"]
    }, {
      "id": 2,
      "usr": "c:type_usage_typedef_and_using.cc@T@Foo2",
      "short_name": "Foo2",
      "qualified_name": "Foo2",
      "definition": "3:13",
      "alias_of": 0,
      "uses": ["*3:13", "*9:14"]
    }, {
      "id": 3,
      "usr": "c:@Foo3",
      "short_name": "Foo3",
      "qualified_name": "Foo3",
      "definition": "4:7",
      "alias_of": 1,
      "uses": ["*4:7", "*10:14"]
    }, {
      "id": 4,
      "usr": "c:@Foo4",
      "short_name": "Foo4",
      "qualified_name": "Foo4",
      "definition": "5:7",
      "uses": ["*5:7"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@accept#*$@S@Foo#",
      "short_name": "accept",
      "qualified_name": "accept",
      "definition": "7:6",
      "uses": ["7:6"]
    }, {
      "id": 1,
      "usr": "c:@F@accept1#**$@S@Foo#",
      "short_name": "accept1",
      "qualified_name": "accept1",
      "definition": "8:6",
      "uses": ["8:6"]
    }, {
      "id": 2,
      "usr": "c:@F@accept2#*$@S@Foo#",
      "short_name": "accept2",
      "qualified_name": "accept2",
      "definition": "9:6",
      "uses": ["9:6"]
    }, {
      "id": 3,
      "usr": "c:@F@accept3#**$@S@Foo#",
      "short_name": "accept3",
      "qualified_name": "accept3",
      "definition": "10:6",
      "uses": ["10:6"]
    }]
}
*/
