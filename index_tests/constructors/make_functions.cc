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
      "kind": 9,
      "storage": 0,
      "declarations": [],
      "spell": "7:3-7:9|14935975554338052500|2|514",
      "extent": "7:3-7:32|14935975554338052500|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 13028995015627606181,
      "detailed_name": "Foobar::Foobar(int)",
      "qual_name_offset": 0,
      "short_name": "Foobar",
      "kind": 9,
      "storage": 0,
      "declarations": [],
      "spell": "6:3-6:9|14935975554338052500|2|514",
      "extent": "6:3-6:17|14935975554338052500|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 13131778807733950299,
      "detailed_name": "Foobar::Foobar()",
      "qual_name_offset": 0,
      "short_name": "Foobar",
      "kind": 9,
      "storage": 0,
      "declarations": [],
      "spell": "5:3-5:9|14935975554338052500|2|514",
      "extent": "5:3-5:14|14935975554338052500|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 17321436359755983845,
      "detailed_name": "Foobar::Foobar(int, Bar *, bool *)",
      "qual_name_offset": 0,
      "short_name": "Foobar",
      "kind": 9,
      "storage": 0,
      "declarations": [],
      "spell": "8:3-8:9|14935975554338052500|2|514",
      "extent": "8:3-8:30|14935975554338052500|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 12993848456528750350,
      "detailed_name": "struct Bar {}",
      "qual_name_offset": 7,
      "short_name": "Bar",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:11|0|1|2",
      "extent": "1:1-1:14|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:17-7:20|0|1|4", "8:15-8:18|0|1|4"]
    }, {
      "usr": 14935975554338052500,
      "detailed_name": "class Foobar {}",
      "qual_name_offset": 6,
      "short_name": "Foobar",
      "kind": 5,
      "declarations": [],
      "spell": "3:7-3:13|0|1|2",
      "extent": "3:1-9:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [13131778807733950299, 13028995015627606181, 3765833212244435302, 17321436359755983845],
      "vars": [],
      "instances": [],
      "uses": ["5:3-5:9|0|1|4", "6:3-6:9|0|1|4", "7:3-7:9|0|1|4", "8:3-8:9|0|1|4"]
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
      "usr": 2532818908869373467,
      "detailed_name": "T *maKE_NoRefs(Args ...args)",
      "qual_name_offset": 3,
      "short_name": "maKE_NoRefs",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "9:4-9:15|0|1|2",
      "extent": "9:1-11:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [3908732770590594660],
      "uses": ["17:3-17:14|0|1|8228"],
      "callees": []
    }, {
      "usr": 2816883305867289955,
      "detailed_name": "void caller22()",
      "qual_name_offset": 5,
      "short_name": "caller22",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "13:6-13:14|0|1|2",
      "extent": "13:1-18:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 15793662558620604611,
      "detailed_name": "T *MakeUnique(Args &&...args)",
      "qual_name_offset": 3,
      "short_name": "MakeUnique",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "4:4-4:14|0|1|2",
      "extent": "4:1-6:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [8463700030555379526],
      "uses": ["14:3-14:13|0|1|8228", "15:3-15:13|0|1|8228", "16:3-16:13|0|1|8228"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 12993848456528750350,
      "detailed_name": "struct Bar {}",
      "qual_name_offset": 7,
      "short_name": "Bar",
      "kind": 23,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["16:29-16:32|0|1|4", "17:30-17:33|0|1|4"]
    }, {
      "usr": 14935975554338052500,
      "detailed_name": "class Foobar {}",
      "qual_name_offset": 6,
      "short_name": "Foobar",
      "kind": 5,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["14:14-14:20|0|1|4", "15:14-15:20|0|1|4", "16:14-16:20|0|1|4", "17:15-17:21|0|1|4"]
    }],
  "usr2var": [{
      "usr": 3908732770590594660,
      "detailed_name": "Args ...args",
      "qual_name_offset": 8,
      "short_name": "args",
      "declarations": [],
      "spell": "9:24-9:28|2532818908869373467|3|514",
      "extent": "9:16-9:28|2532818908869373467|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 0
    }, {
      "usr": 8463700030555379526,
      "detailed_name": "Args &&...args",
      "qual_name_offset": 10,
      "short_name": "args",
      "declarations": [],
      "spell": "4:25-4:29|15793662558620604611|3|514",
      "extent": "4:15-4:29|15793662558620604611|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 0
    }]
}
*/
