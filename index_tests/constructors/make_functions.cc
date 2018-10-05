#include "make_functions.h"

template <typename T, typename... Args>
T* MakeUnique(Args&&... args) {
  return nullptr;
}

template <typename T, typename... Args>
T* maKE_NoRefs(Args... args) {
  return nullptr;
}

void caller22() {
  MakeUnique<Foobar>();
  MakeUnique<Foobar>(1);
  MakeUnique<Foobar>(1, new Bar(), nullptr);
  maKE_NoRefs<Foobar>(1, new Bar(), nullptr);
}

// TODO: Eliminate the extra entries in the "types" array here. They come from
// the template function definitions.

// Foobar is defined in a separate file to ensure that we can attribute
// MakeUnique calls across translation units.

/*
OUTPUT: make_functions.h
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3765833212244435302,
      "detailed_name": "Foobar::Foobar(int &&, Bar *, bool *)",
      "qual_name_offset": 0,
      "short_name": "Foobar",
      "spell": "7:3-7:9|7:3-7:32|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 13028995015627606181,
      "detailed_name": "Foobar::Foobar(int)",
      "qual_name_offset": 0,
      "short_name": "Foobar",
      "spell": "6:3-6:9|6:3-6:17|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 13131778807733950299,
      "detailed_name": "Foobar::Foobar()",
      "qual_name_offset": 0,
      "short_name": "Foobar",
      "spell": "5:3-5:9|5:3-5:14|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 17321436359755983845,
      "detailed_name": "Foobar::Foobar(int, Bar *, bool *)",
      "qual_name_offset": 0,
      "short_name": "Foobar",
      "spell": "8:3-8:9|8:3-8:30|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 12993848456528750350,
      "detailed_name": "struct Bar {}",
      "qual_name_offset": 7,
      "short_name": "Bar",
      "spell": "1:8-1:11|1:1-1:14|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["7:17-7:20|4|-1", "8:15-8:18|4|-1"]
    }, {
      "usr": 14935975554338052500,
      "detailed_name": "class Foobar {}",
      "qual_name_offset": 6,
      "short_name": "Foobar",
      "spell": "3:7-3:13|3:1-9:2|2|-1",
      "bases": [],
      "funcs": [13131778807733950299, 13028995015627606181, 3765833212244435302, 17321436359755983845],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["5:3-5:9|4|-1", "6:3-6:9|4|-1", "7:3-7:9|4|-1", "8:3-8:9|4|-1"]
    }],
  "usr2var": []
}
OUTPUT: make_functions.cc
{
  "includes": [{
      "line": 0,
      "resolved_path": "&make_functions.h"
    }],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 768523651983844320,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "vars": [2555873744476712860, 2555873744476712860, 2555873744476712860],
      "callees": [],
      "kind": 0,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 2532818908869373467,
      "detailed_name": "T *maKE_NoRefs(Args ...args)",
      "qual_name_offset": 3,
      "short_name": "maKE_NoRefs",
      "spell": "9:4-9:15|9:1-11:2|2|-1",
      "bases": [],
      "vars": [3908732770590594660],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["17:3-17:14|16420|-1"]
    }, {
      "usr": 2816883305867289955,
      "detailed_name": "void caller22()",
      "qual_name_offset": 5,
      "short_name": "caller22",
      "spell": "13:6-13:14|13:1-18:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["14:3-14:13|15793662558620604611|3|16420", "15:3-15:13|15793662558620604611|3|16420", "16:3-16:13|15793662558620604611|3|16420", "17:3-17:14|2532818908869373467|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 11138976705878544996,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "vars": [16395392342608151399],
      "callees": [],
      "kind": 0,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 11363675606380070883,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "vars": [180270746871803062, 180270746871803062, 180270746871803062],
      "callees": [],
      "kind": 0,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 15793662558620604611,
      "detailed_name": "T *MakeUnique(Args &&...args)",
      "qual_name_offset": 3,
      "short_name": "MakeUnique",
      "spell": "4:4-4:14|4:1-6:2|2|-1",
      "bases": [],
      "vars": [8463700030555379526],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["14:3-14:13|16420|-1", "15:3-15:13|16420|-1", "16:3-16:13|16420|-1"]
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [180270746871803062],
      "uses": []
    }, {
      "usr": 87,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [180270746871803062],
      "uses": []
    }, {
      "usr": 12993848456528750350,
      "detailed_name": "struct Bar {}",
      "qual_name_offset": 7,
      "short_name": "Bar",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["16:29-16:32|4|-1", "17:30-17:33|4|-1"]
    }, {
      "usr": 14935975554338052500,
      "detailed_name": "class Foobar {}",
      "qual_name_offset": 6,
      "short_name": "Foobar",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["14:14-14:20|4|-1", "15:14-15:20|4|-1", "16:14-16:20|4|-1", "17:15-17:21|4|-1"]
    }],
  "usr2var": [{
      "usr": 180270746871803062,
      "detailed_name": "int args",
      "qual_name_offset": 4,
      "short_name": "args",
      "spell": "9:24-9:28|9:16-9:28|1026|-1",
      "type": 87,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 2555873744476712860,
      "detailed_name": "int &&args",
      "qual_name_offset": 6,
      "short_name": "args",
      "spell": "4:25-4:29|4:15-4:29|1026|-1",
      "type": 0,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 3908732770590594660,
      "detailed_name": "Args ...args",
      "qual_name_offset": 8,
      "short_name": "args",
      "spell": "9:24-9:28|9:16-9:28|1026|-1",
      "type": 0,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 8463700030555379526,
      "detailed_name": "Args &&...args",
      "qual_name_offset": 10,
      "short_name": "args",
      "spell": "4:25-4:29|4:15-4:29|1026|-1",
      "type": 0,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16395392342608151399,
      "detailed_name": "int &&args",
      "qual_name_offset": 6,
      "short_name": "args",
      "spell": "4:25-4:29|4:15-4:29|1026|-1",
      "type": 0,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
