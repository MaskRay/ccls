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
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 12993848456528750350,
      "detailed_name": "Bar",
      "short_name": "Bar",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:11|-1|1|2",
      "extent": "1:1-1:14|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:17-7:20|-1|1|4", "8:15-8:18|-1|1|4"]
    }, {
      "id": 1,
      "usr": 14935975554338052500,
      "detailed_name": "Foobar",
      "short_name": "Foobar",
      "kind": 5,
      "declarations": ["5:3-5:9|-1|1|4", "6:3-6:9|-1|1|4", "7:3-7:9|-1|1|4", "8:3-8:9|-1|1|4"],
      "spell": "3:7-3:13|-1|1|2",
      "extent": "3:1-9:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0, 1, 2, 3],
      "vars": [],
      "instances": [],
      "uses": ["5:3-5:9|1|2|4", "6:3-6:9|1|2|4", "7:3-7:9|1|2|4", "8:3-8:9|1|2|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 13131778807733950299,
      "detailed_name": "void Foobar::Foobar()",
      "short_name": "Foobar",
      "kind": 9,
      "storage": 1,
      "declarations": [],
      "spell": "5:3-5:9|1|2|2",
      "extent": "5:3-5:14|1|2|0",
      "declaring_type": 1,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 1,
      "usr": 13028995015627606181,
      "detailed_name": "void Foobar::Foobar(int)",
      "short_name": "Foobar",
      "kind": 9,
      "storage": 1,
      "declarations": [],
      "spell": "6:3-6:9|1|2|2",
      "extent": "6:3-6:17|1|2|0",
      "declaring_type": 1,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 2,
      "usr": 3765833212244435302,
      "detailed_name": "void Foobar::Foobar(int &&, Bar *, bool *)",
      "short_name": "Foobar",
      "kind": 9,
      "storage": 1,
      "declarations": [],
      "spell": "7:3-7:9|1|2|2",
      "extent": "7:3-7:32|1|2|0",
      "declaring_type": 1,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 3,
      "usr": 17321436359755983845,
      "detailed_name": "void Foobar::Foobar(int, Bar *, bool *)",
      "short_name": "Foobar",
      "kind": 9,
      "storage": 1,
      "declarations": [],
      "spell": "8:3-8:9|1|2|2",
      "extent": "8:3-8:30|1|2|0",
      "declaring_type": 1,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
OUTPUT: make_functions.cc
{
  "includes": [{
      "line": 0,
      "resolved_path": "&make_functions.h"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 7902098450755788854,
      "detailed_name": "T",
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "3:20-3:21|0|3|2",
      "extent": "3:11-3:21|0|3|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:1-4:2|-1|1|4"]
    }, {
      "id": 1,
      "usr": 12533159752419999454,
      "detailed_name": "Args",
      "short_name": "Args",
      "kind": 26,
      "declarations": [],
      "spell": "3:35-3:39|0|3|2",
      "extent": "3:23-3:39|0|3|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:15-4:19|-1|1|4"]
    }, {
      "id": 2,
      "usr": 18441628706991062891,
      "detailed_name": "T",
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "8:20-8:21|1|3|2",
      "extent": "8:11-8:21|1|3|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:1-9:2|-1|1|4"]
    }, {
      "id": 3,
      "usr": 9441341235704820385,
      "detailed_name": "Args",
      "short_name": "Args",
      "kind": 26,
      "declarations": [],
      "spell": "8:35-8:39|1|3|2",
      "extent": "8:23-8:39|1|3|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:16-9:20|-1|1|4"]
    }, {
      "id": 4,
      "usr": 14935975554338052500,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["14:14-14:20|-1|1|4", "15:14-15:20|-1|1|4", "16:14-16:20|-1|1|4", "17:15-17:21|-1|1|4"]
    }, {
      "id": 5,
      "usr": 12993848456528750350,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["16:29-16:32|-1|1|4", "17:30-17:33|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 15793662558620604611,
      "detailed_name": "T *MakeUnique(Args &&... args)",
      "short_name": "MakeUnique",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "4:4-4:14|-1|1|2",
      "extent": "4:1-6:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0],
      "uses": ["14:3-14:13|2|3|32", "15:3-15:13|2|3|32", "16:3-16:13|2|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 2532818908869373467,
      "detailed_name": "T *maKE_NoRefs(Args... args)",
      "short_name": "maKE_NoRefs",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "9:4-9:15|-1|1|2",
      "extent": "9:1-11:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [1],
      "uses": ["17:3-17:14|2|3|32"],
      "callees": []
    }, {
      "id": 2,
      "usr": 2816883305867289955,
      "detailed_name": "void caller22()",
      "short_name": "caller22",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "13:6-13:14|-1|1|2",
      "extent": "13:1-18:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["14:3-14:13|0|3|32", "15:3-15:13|0|3|32", "16:3-16:13|0|3|32", "17:3-17:14|1|3|32"]
    }, {
      "id": 3,
      "usr": 13131778807733950299,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["14:3-14:13|-1|1|288"],
      "callees": []
    }, {
      "id": 4,
      "usr": 13028995015627606181,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["15:3-15:13|-1|1|288"],
      "callees": []
    }, {
      "id": 5,
      "usr": 3765833212244435302,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["16:3-16:13|-1|1|288"],
      "callees": []
    }, {
      "id": 6,
      "usr": 17321436359755983845,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["17:3-17:14|-1|1|288"],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 15288691366352169805,
      "detailed_name": "Args &&... args",
      "short_name": "args",
      "declarations": [],
      "spell": "4:25-4:29|0|3|2",
      "extent": "4:15-4:29|0|3|0",
      "uses": [],
      "kind": 253,
      "storage": 1
    }, {
      "id": 1,
      "usr": 12338908251430965107,
      "detailed_name": "Args... args",
      "short_name": "args",
      "declarations": [],
      "spell": "9:24-9:28|1|3|2",
      "extent": "9:16-9:28|1|3|0",
      "uses": [],
      "kind": 253,
      "storage": 1
    }]
}
*/
