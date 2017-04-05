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
      "uses": ["1:8-1:11", "*2:14-2:17", "*3:9-3:12", "*7:13-7:16"]
    }, {
      "id": 1,
      "usr": "c:@Foo1",
      "short_name": "Foo1",
      "qualified_name": "Foo1",
      "definition_spelling": "2:7-2:11",
      "definition_extent": "2:1-2:18",
      "alias_of": 0,
      "uses": ["*2:7-2:11", "*4:14-4:18", "*8:14-8:18"]
    }, {
      "id": 2,
      "usr": "c:type_usage_typedef_and_using.cc@T@Foo2",
      "short_name": "Foo2",
      "qualified_name": "Foo2",
      "definition_spelling": "3:13-3:17",
      "definition_extent": "3:1-3:17",
      "alias_of": 0,
      "uses": ["*3:13-3:17", "*9:14-9:18"]
    }, {
      "id": 3,
      "usr": "c:@Foo3",
      "short_name": "Foo3",
      "qualified_name": "Foo3",
      "definition_spelling": "4:7-4:11",
      "definition_extent": "4:1-4:18",
      "alias_of": 1,
      "uses": ["*4:7-4:11", "*10:14-10:18"]
    }, {
      "id": 4,
      "usr": "c:@Foo4",
      "short_name": "Foo4",
      "qualified_name": "Foo4",
      "definition_spelling": "5:7-5:11",
      "definition_extent": "5:1-5:17",
      "uses": ["*5:7-5:11"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@accept#*$@S@Foo#",
      "short_name": "accept",
      "qualified_name": "accept",
      "definition_spelling": "7:6-7:12",
      "definition_extent": "7:1-7:21",
      "uses": ["7:6-7:12"]
    }, {
      "id": 1,
      "usr": "c:@F@accept1#**$@S@Foo#",
      "short_name": "accept1",
      "qualified_name": "accept1",
      "definition_spelling": "8:6-8:13",
      "definition_extent": "8:1-8:23",
      "uses": ["8:6-8:13"]
    }, {
      "id": 2,
      "usr": "c:@F@accept2#*$@S@Foo#",
      "short_name": "accept2",
      "qualified_name": "accept2",
      "definition_spelling": "9:6-9:13",
      "definition_extent": "9:1-9:23",
      "uses": ["9:6-9:13"]
    }, {
      "id": 3,
      "usr": "c:@F@accept3#**$@S@Foo#",
      "short_name": "accept3",
      "qualified_name": "accept3",
      "definition_spelling": "10:6-10:13",
      "definition_extent": "10:1-10:23",
      "uses": ["10:6-10:13"]
    }]
}
*/
