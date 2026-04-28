template <typename T, typename... Args> T *fake_make_unique(Args &&...args) {
  return new T(static_cast<Args &&>(args)...);
}

struct A {
  A();
  A(int);
  A(int, int);
};

void caller() {
  fake_make_unique<A>();
  fake_make_unique<A>(1);
  fake_make_unique<A>(1, 2);
}

// Each constructor should have a use at the matching call site (e.g. A::A()
// at line 13, A::A(int) at line 14, A::A(int, int) at line 15) in addition
// to the body location 3:14, with Role::Implicit (role bits include 256).
// `caller` should have callees pointing to the constructors as well as to
// fake_make_unique itself.

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4887303696598544169,
      "detailed_name": "A::A()",
      "qual_name_offset": 0,
      "short_name": "A",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["6:3-6:4|6:3-6:6|1025|-1"],
      "derived": [],
      "uses": ["2:14-2:15|16676|-1", "12:3-12:19|16676|-1"]
    }, {
      "usr": 6249285646691162571,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "vars": [],
      "callees": ["2:14-2:15|4887303696598544169|3|16676"],
      "kind": 0,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 9129661816613481765,
      "detailed_name": "T *fake_make_unique(Args &&...args)",
      "qual_name_offset": 3,
      "short_name": "fake_make_unique",
      "spell": "1:44-1:60|1:41-3:2|2|-1",
      "bases": [],
      "vars": [17268248497583492412],
      "callees": [],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["12:3-12:19|16420|-1", "13:3-13:19|16420|-1", "14:3-14:19|16420|-1"]
    }, {
      "usr": 9380760492144642535,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "vars": [10186585723342590949],
      "callees": ["2:14-2:15|11379910413970621988|3|16676"],
      "kind": 0,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 11379910413970621988,
      "detailed_name": "A::A(int)",
      "qual_name_offset": 0,
      "short_name": "A",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["7:3-7:4|7:3-7:9|1025|-1"],
      "derived": [],
      "uses": ["2:14-2:15|16676|-1", "13:3-13:19|16676|-1"]
    }, {
      "usr": 11404881820527069090,
      "detailed_name": "void caller()",
      "qual_name_offset": 5,
      "short_name": "caller",
      "spell": "11:6-11:12|11:1-15:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["12:3-12:19|9129661816613481765|3|16420", "13:3-13:19|9129661816613481765|3|16420",
"14:3-14:19|9129661816613481765|3|16420", "12:3-12:19|4887303696598544169|3|16676",
"13:3-13:19|11379910413970621988|3|16676", "14:3-14:19|16584514099482440453|3|16676"], "kind": 12, "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 12753818000817661723,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "vars": [7421893224715895136, 7421893224715895136],
      "callees": ["2:14-2:15|16584514099482440453|3|16676"],
      "kind": 0,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 16584514099482440453,
      "detailed_name": "A::A(int, int)",
      "qual_name_offset": 0,
      "short_name": "A",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["8:3-8:4|8:3-8:14|1025|-1"],
      "derived": [],
      "uses": ["2:14-2:15|16676|-1", "14:3-14:19|16676|-1"]
    }],
  "usr2type": [{
      "usr": 1265876668139249013,
      "detailed_name": "struct A {}",
      "qual_name_offset": 7,
      "short_name": "A",
      "spell": "5:8-5:9|5:1-9:2|2|-1",
      "bases": [],
      "funcs": [4887303696598544169, 11379910413970621988, 16584514099482440453],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["6:3-6:4|4|-1", "7:3-7:4|4|-1", "8:3-8:4|4|-1", "12:20-12:21|4|-1", "13:20-13:21|4|-1",
"14:20-14:21|4|-1"]
    }, {
      "usr": 1610023536658236104,
      "detailed_name": "Args",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 0,
      "declarations": ["1:35-1:39|1:23-1:39|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["1:61-1:65|4|-1", "2:28-2:32|4|-1"]
    }, {
      "usr": 11194248619456103048,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 0,
      "declarations": ["1:20-1:21|1:11-1:21|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["1:41-1:42|4|-1", "2:14-2:15|4|-1"]
    }],
  "usr2var": [{
      "usr": 7421893224715895136,
      "detailed_name": "int &&args",
      "qual_name_offset": 6,
      "short_name": "args",
      "spell": "1:71-1:75|1:61-1:75|1026|-1",
      "type": 0,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["2:37-2:41|12|-1"]
    }, {
      "usr": 10186585723342590949,
      "detailed_name": "int &&args",
      "qual_name_offset": 6,
      "short_name": "args",
      "spell": "1:71-1:75|1:61-1:75|1026|-1",
      "type": 0,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["2:37-2:41|12|-1"]
    }, {
      "usr": 17268248497583492412,
      "detailed_name": "Args &&...args",
      "qual_name_offset": 10,
      "short_name": "args",
      "spell": "1:71-1:75|1:61-1:75|1026|-1",
      "type": 0,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["2:37-2:41|4|-1"]
    }]
}
*/
