struct Foo {};
struct Bar {};
using Alias = Foo;

// Only instantiation: x is Foo.
template <typename T> void one(T t) {
  auto x = t;
}
// Two instantiations: y stays auto.
template <typename T> void two(T t) {
  auto y = t;
}

// Instantiated early (deduced return type) and again later: w stays auto.
namespace N {
template <typename T> auto three(T t) {
  auto w = t;
  return w;
}
Foo foo = three(Foo());
} // namespace N
Bar bar = N::three(Bar());

void f() {
  one(Foo());
  two(Foo());
  two(Bar());
  auto z = Alias();
}

// Locals and parameters of the instantiations are not indexed separately.

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 880549676430489861,
      "detailed_name": "void f()",
      "qual_name_offset": 5,
      "short_name": "f",
      "spell": "24:6-24:7|24:1-29:2|2|-1",
      "bases": [],
      "vars": [8880916417959871896],
      "callees": ["25:3-25:6|3293824179844408963|3|16420", "26:3-26:6|15012685172267614675|3|16420", "27:3-27:6|15012685172267614675|3|16420"],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 3293824179844408963,
      "detailed_name": "void one(T t)",
      "qual_name_offset": 5,
      "short_name": "one",
      "spell": "6:28-6:31|6:23-8:2|2|-1",
      "comments": "Only instantiation: x is Foo.",
      "bases": [],
      "vars": [12404099028783649473, 13783755614533241778],
      "callees": [],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["25:3-25:6|16420|-1"]
    }, {
      "usr": 4176498977223578736,
      "detailed_name": "auto N::three(T t)",
      "qual_name_offset": 5,
      "short_name": "three",
      "spell": "16:28-16:33|16:23-19:2|1026|-1",
      "bases": [],
      "vars": [7038208679121361490, 14937771426251528095],
      "callees": [],
      "kind": 12,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["20:11-20:16|36|-1", "22:14-22:19|36|-1"]
    }, {
      "usr": 15012685172267614675,
      "detailed_name": "void two(T t)",
      "qual_name_offset": 5,
      "short_name": "two",
      "spell": "10:28-10:31|10:23-12:2|2|-1",
      "comments": "Two instantiations: y stays auto.",
      "bases": [],
      "vars": [7203469399117519765, 4814426079409952428],
      "callees": [],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["26:3-26:6|16420|-1", "27:3-27:6|16420|-1"]
    }],
  "usr2type": [{
      "usr": 124821069060960122,
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
      "declarations": ["10:20-10:21|10:11-10:21|1|-1"],
      "derived": [],
      "instances": [7203469399117519765],
      "uses": ["10:32-10:33|4|-1"]
    }, {
      "usr": 4983383778661989348,
      "detailed_name": "namespace N {}",
      "qual_name_offset": 10,
      "short_name": "N",
      "comments": "Instantiated early (deduced return type) and again later: w stays auto.",
      "bases": [],
      "funcs": [4176498977223578736],
      "types": [],
      "vars": [{
          "L": 7428939199205028218,
          "R": -1
        }],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["15:11-15:12|15:1-21:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["22:11-22:12|4|-1"]
    }, {
      "usr": 5907654173228278707,
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
      "declarations": ["16:20-16:21|16:11-16:21|1|-1"],
      "derived": [],
      "instances": [7038208679121361490],
      "uses": ["16:34-16:35|4|-1"]
    }, {
      "usr": 8728279549108618675,
      "detailed_name": "using Alias = Foo",
      "qual_name_offset": 6,
      "short_name": "Alias",
      "spell": "3:7-3:12|3:1-3:18|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 15041163540773201510,
      "kind": 252,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["28:12-28:17|4|-1"]
    }, {
      "usr": 12993848456528750350,
      "detailed_name": "struct Bar {}",
      "qual_name_offset": 7,
      "short_name": "Bar",
      "spell": "2:8-2:11|2:1-2:14|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [957939736636282080],
      "uses": ["22:1-22:4|4|-1", "22:20-22:23|4|-1", "27:7-27:10|4|-1"]
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "1:8-1:11|1:1-1:14|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [7428939199205028218, 8880916417959871896, 13783755614533241778],
      "uses": ["3:15-3:18|4|-1", "20:1-20:4|4|-1", "20:17-20:20|4|-1", "25:7-25:10|4|-1", "26:7-26:10|4|-1"]
    }, {
      "usr": 16844043143661229910,
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
      "declarations": ["6:20-6:21|6:11-6:21|1|-1"],
      "derived": [],
      "instances": [12404099028783649473],
      "uses": ["6:32-6:33|4|-1"]
    }],
  "usr2var": [{
      "usr": 957939736636282080,
      "detailed_name": "Bar bar",
      "qual_name_offset": 4,
      "short_name": "bar",
      "hover": "Bar bar = N::three(Bar())",
      "spell": "22:5-22:8|22:1-22:26|2|-1",
      "type": 12993848456528750350,
      "kind": 13,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 4814426079409952428,
      "detailed_name": "auto y",
      "qual_name_offset": 5,
      "short_name": "y",
      "hover": "auto y = t",
      "spell": "11:8-11:9|11:3-11:13|2|-1",
      "type": 0,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 7038208679121361490,
      "detailed_name": "T t",
      "qual_name_offset": 2,
      "short_name": "t",
      "spell": "16:36-16:37|16:34-16:37|1026|-1",
      "type": 5907654173228278707,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["17:12-17:13|4|-1"]
    }, {
      "usr": 7203469399117519765,
      "detailed_name": "T t",
      "qual_name_offset": 2,
      "short_name": "t",
      "spell": "10:34-10:35|10:32-10:35|1026|-1",
      "type": 124821069060960122,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["11:12-11:13|4|-1"]
    }, {
      "usr": 7428939199205028218,
      "detailed_name": "Foo N::foo",
      "qual_name_offset": 4,
      "short_name": "foo",
      "hover": "Foo N::foo = three(Foo())",
      "spell": "20:5-20:8|20:1-20:23|1026|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 8880916417959871896,
      "detailed_name": "Alias z",
      "qual_name_offset": 6,
      "short_name": "z",
      "hover": "Alias z = Alias()\n// aka Foo",
      "spell": "28:8-28:9|28:3-28:19|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 12404099028783649473,
      "detailed_name": "T t",
      "qual_name_offset": 2,
      "short_name": "t",
      "spell": "6:34-6:35|6:32-6:35|1026|-1",
      "type": 16844043143661229910,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["7:12-7:13|4|-1"]
    }, {
      "usr": 13783755614533241778,
      "detailed_name": "Foo x",
      "qual_name_offset": 4,
      "short_name": "x",
      "hover": "Foo x = t",
      "spell": "7:8-7:9|7:3-7:13|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 14937771426251528095,
      "detailed_name": "auto w",
      "qual_name_offset": 5,
      "short_name": "w",
      "hover": "auto w = t",
      "spell": "17:8-17:9|17:3-17:13|2|-1",
      "type": 0,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["18:10-18:11|4|-1"]
    }]
}
*/
