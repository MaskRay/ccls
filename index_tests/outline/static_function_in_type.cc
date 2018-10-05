#include "static_function_in_type.h"

namespace ns {
// static
void Foo::Register(Manager* m) {
}
}

/*
OUTPUT: static_function_in_type.h
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 17019747379608639279,
      "detailed_name": "static void ns::Foo::Register(ns::Manager *)",
      "qual_name_offset": 12,
      "short_name": "Register",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 254,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["6:15-6:23|6:3-6:33|1025|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 1972401196751872203,
      "detailed_name": "class ns::Manager",
      "qual_name_offset": 6,
      "short_name": "Manager",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": ["3:7-3:14|3:1-3:14|1025|-1"],
      "derived": [],
      "instances": [],
      "uses": ["6:24-6:31|4|-1"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "bases": [],
      "funcs": [],
      "types": [1972401196751872203, 17262466801709381811],
      "vars": [],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["1:11-1:13|1:1-9:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 17262466801709381811,
      "detailed_name": "struct ns::Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "5:8-5:11|5:1-7:2|1026|-1",
      "bases": [],
      "funcs": [17019747379608639279],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 3,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
OUTPUT: static_function_in_type.cc
{
  "includes": [{
      "line": 0,
      "resolved_path": "&static_function_in_type.h"
    }],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 17019747379608639279,
      "detailed_name": "static void ns::Foo::Register(ns::Manager *)",
      "qual_name_offset": 12,
      "short_name": "Register",
      "spell": "5:11-5:19|5:1-6:2|1026|-1",
      "comments": "static",
      "bases": [],
      "vars": [13569879755236306838],
      "callees": [],
      "kind": 254,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 1972401196751872203,
      "detailed_name": "class ns::Manager",
      "qual_name_offset": 6,
      "short_name": "Manager",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [13569879755236306838],
      "uses": ["5:20-5:27|4|-1"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["3:11-3:13|3:1-7:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 17262466801709381811,
      "detailed_name": "struct ns::Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "bases": [],
      "funcs": [17019747379608639279],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["5:6-5:9|4|-1"]
    }],
  "usr2var": [{
      "usr": 13569879755236306838,
      "detailed_name": "ns::Manager *m",
      "qual_name_offset": 13,
      "short_name": "m",
      "spell": "5:29-5:30|5:20-5:30|1026|-1",
      "type": 1972401196751872203,
      "kind": 253,
      "parent_kind": 254,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/