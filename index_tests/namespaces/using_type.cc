namespace ns {
struct A {};
typedef A T;
}
using ns::A;
using ns::T;
void foo() {
  A a;
  T t;
}
struct B : A {};
struct C : T {};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "7:6-7:9|7:1-10:2|2|-1",
      "bases": [],
      "vars": [624270373391590559, 6399810688002890805],
      "callees": [],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 1339685772751878249,
      "detailed_name": "typedef A ns::T",
      "qual_name_offset": 10,
      "short_name": "T",
      "spell": "3:11-3:12|3:1-3:12|1026|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 4263703702042956015,
      "kind": 252,
      "parent_kind": 3,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["6:11-6:12|4|-1", "9:3-9:4|4|-1", "12:12-12:13|4|-1"]
    }, {
      "usr": 4263703702042956015,
      "detailed_name": "struct ns::A {}",
      "qual_name_offset": 7,
      "short_name": "A",
      "spell": "2:8-2:9|2:1-2:12|1026|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 3,
      "declarations": [],
      "derived": [12245402492060663523, 18277030710307801909],
      "instances": [624270373391590559, 6399810688002890805],
      "uses": ["3:9-3:10|4|-1", "5:11-5:12|4|-1", "8:3-8:4|4|-1", "11:12-11:13|2052|-1", "12:12-12:13|2308|-1"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "bases": [],
      "funcs": [],
      "types": [4263703702042956015, 1339685772751878249],
      "vars": [],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["1:11-1:13|1:1-4:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["5:7-5:9|4|-1", "6:7-6:9|4|-1"]
    }, {
      "usr": 12245402492060663523,
      "detailed_name": "struct B : A {}",
      "qual_name_offset": 7,
      "short_name": "B",
      "spell": "11:8-11:9|11:1-11:16|2|-1",
      "bases": [4263703702042956015],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 18277030710307801909,
      "detailed_name": "struct C : T {}",
      "qual_name_offset": 7,
      "short_name": "C",
      "spell": "12:8-12:9|12:1-12:16|2|-1",
      "bases": [4263703702042956015],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 624270373391590559,
      "detailed_name": "A a",
      "qual_name_offset": 2,
      "short_name": "a",
      "spell": "8:5-8:6|8:3-8:6|2|-1",
      "type": 4263703702042956015,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 6399810688002890805,
      "detailed_name": "T t",
      "qual_name_offset": 2,
      "short_name": "t",
      "spell": "9:5-9:6|9:3-9:6|2|-1",
      "type": 4263703702042956015,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
