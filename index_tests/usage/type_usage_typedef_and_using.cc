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
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 558620830317390922,
      "detailed_name": "void accept1(Foo1 *)",
      "qual_name_offset": 5,
      "short_name": "accept1",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "8:6-8:13|0|1|2",
      "extent": "8:1-8:23|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 9119341505144503905,
      "detailed_name": "void accept(Foo *)",
      "qual_name_offset": 5,
      "short_name": "accept",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "7:6-7:12|0|1|2",
      "extent": "7:1-7:21|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 10523262907746124479,
      "detailed_name": "void accept2(Foo2 *)",
      "qual_name_offset": 5,
      "short_name": "accept2",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "9:6-9:13|0|1|2",
      "extent": "9:1-9:23|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 14986366321326974406,
      "detailed_name": "void accept3(Foo3 *)",
      "qual_name_offset": 5,
      "short_name": "accept3",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "10:6-10:13|0|1|2",
      "extent": "10:1-10:23|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 17,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 1544499294580512394,
      "detailed_name": "Foo1",
      "qual_name_offset": 0,
      "short_name": "Foo1",
      "kind": 252,
      "hover": "using Foo1 = Foo*",
      "declarations": [],
      "spell": "2:7-2:11|0|1|2",
      "extent": "2:1-2:18|0|1|0",
      "alias_of": 15041163540773201510,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:7-2:11|0|1|4", "4:14-4:18|0|1|4", "8:14-8:18|0|1|4"]
    }, {
      "usr": 2638219001294786365,
      "detailed_name": "Foo4",
      "qual_name_offset": 0,
      "short_name": "Foo4",
      "kind": 252,
      "hover": "using Foo4 = int",
      "declarations": [],
      "spell": "5:7-5:11|0|1|2",
      "extent": "5:1-5:17|0|1|0",
      "alias_of": 17,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["5:7-5:11|0|1|4"]
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 23,
      "declarations": ["1:8-1:11|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:14-2:17|0|1|4", "3:9-3:12|0|1|4", "7:13-7:16|0|1|4"]
    }, {
      "usr": 15466821155413653804,
      "detailed_name": "Foo2",
      "qual_name_offset": 0,
      "short_name": "Foo2",
      "kind": 252,
      "hover": "typedef Foo Foo2",
      "declarations": [],
      "spell": "3:13-3:17|0|1|2",
      "extent": "3:1-3:17|0|1|0",
      "alias_of": 15041163540773201510,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["3:13-3:17|0|1|4", "9:14-9:18|0|1|4"]
    }, {
      "usr": 17897026942631673064,
      "detailed_name": "Foo3",
      "qual_name_offset": 0,
      "short_name": "Foo3",
      "kind": 252,
      "hover": "using Foo3 = Foo1",
      "declarations": [],
      "spell": "4:7-4:11|0|1|2",
      "extent": "4:1-4:18|0|1|0",
      "alias_of": 1544499294580512394,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:7-4:11|0|1|4", "10:14-10:18|0|1|4"]
    }],
  "usr2var": []
}
*/
