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
      "kind": 6,
      "storage": 0,
      "declarations": ["6:15-6:23|17262466801709381811|2|1025"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 1972401196751872203,
      "detailed_name": "class ns::Manager",
      "qual_name_offset": 6,
      "short_name": "Manager",
      "kind": 5,
      "declarations": ["3:7-3:14|11072669167287398027|2|1025"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["6:24-6:31|11072669167287398027|2|4"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {\n}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "kind": 3,
      "declarations": ["1:11-1:13|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [1972401196751872203, 17262466801709381811],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 17262466801709381811,
      "detailed_name": "struct ns::Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "5:8-5:11|11072669167287398027|2|1026",
      "extent": "5:1-7:2|11072669167287398027|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [17019747379608639279],
      "vars": [],
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
      "detailed_name": "void Foo::Register(ns::Manager *m)",
      "qual_name_offset": 5,
      "short_name": "Register",
      "kind": 6,
      "storage": 0,
      "comments": "static",
      "declarations": [],
      "spell": "5:11-5:19|17262466801709381811|2|1026",
      "extent": "5:1-6:2|11072669167287398027|2|0",
      "bases": [],
      "derived": [],
      "vars": [13569879755236306838],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 1972401196751872203,
      "detailed_name": "class ns::Manager",
      "qual_name_offset": 6,
      "short_name": "Manager",
      "kind": 5,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [13569879755236306838],
      "uses": ["5:20-5:27|11072669167287398027|2|4"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {\n}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "kind": 3,
      "declarations": ["3:11-3:13|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 17262466801709381811,
      "detailed_name": "struct ns::Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [17019747379608639279],
      "vars": [],
      "instances": [],
      "uses": ["5:6-5:9|11072669167287398027|2|4"]
    }],
  "usr2var": [{
      "usr": 13569879755236306838,
      "detailed_name": "ns::Manager *m",
      "qual_name_offset": 13,
      "short_name": "m",
      "declarations": [],
      "spell": "5:29-5:30|17019747379608639279|3|1026",
      "extent": "5:20-5:30|17019747379608639279|3|0",
      "type": 1972401196751872203,
      "uses": [],
      "kind": 253,
      "storage": 0
    }]
}
*/