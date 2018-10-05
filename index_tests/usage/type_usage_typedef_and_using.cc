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
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 558620830317390922,
      "detailed_name": "void accept1(Foo1 *)",
      "qual_name_offset": 5,
      "short_name": "accept1",
      "spell": "8:6-8:13|8:1-8:23|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 9119341505144503905,
      "detailed_name": "void accept(Foo *)",
      "qual_name_offset": 5,
      "short_name": "accept",
      "spell": "7:6-7:12|7:1-7:21|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 10523262907746124479,
      "detailed_name": "void accept2(Foo2 *)",
      "qual_name_offset": 5,
      "short_name": "accept2",
      "spell": "9:6-9:13|9:1-9:23|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 14986366321326974406,
      "detailed_name": "void accept3(Foo3 *)",
      "qual_name_offset": 5,
      "short_name": "accept3",
      "spell": "10:6-10:13|10:1-10:23|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 1544499294580512394,
      "detailed_name": "using Foo1 = Foo *",
      "qual_name_offset": 6,
      "short_name": "Foo1",
      "spell": "2:7-2:11|2:1-2:18|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 15041163540773201510,
      "kind": 252,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["4:14-4:18|4|-1", "8:14-8:18|4|-1"]
    }, {
      "usr": 2638219001294786365,
      "detailed_name": "using Foo4 = int",
      "qual_name_offset": 6,
      "short_name": "Foo4",
      "spell": "5:7-5:11|5:1-5:17|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 252,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": ["1:8-1:11|1:1-1:11|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["2:14-2:17|4|-1", "3:9-3:12|4|-1", "7:13-7:16|4|-1"]
    }, {
      "usr": 15466821155413653804,
      "detailed_name": "typedef Foo Foo2",
      "qual_name_offset": 12,
      "short_name": "Foo2",
      "spell": "3:13-3:17|3:1-3:17|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 15041163540773201510,
      "kind": 252,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["9:14-9:18|4|-1"]
    }, {
      "usr": 17897026942631673064,
      "detailed_name": "using Foo3 = Foo1",
      "qual_name_offset": 6,
      "short_name": "Foo3",
      "spell": "4:7-4:11|4:1-4:18|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 1544499294580512394,
      "kind": 252,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["10:14-10:18|4|-1"]
    }],
  "usr2var": []
}
*/
