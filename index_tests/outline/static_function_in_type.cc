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
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 17019747379608639279,
      "detailed_name": "void ns::Foo::Register(ns::Manager *)",
      "qual_name_offset": 5,
      "short_name": "Register",
      "kind": 254,
      "storage": 3,
      "declarations": ["6:15-6:23|17262466801709381811|2|1"],
      "declaring_type": 17262466801709381811,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 1972401196751872203,
      "detailed_name": "ns::Manager",
      "qual_name_offset": 0,
      "short_name": "Manager",
      "kind": 5,
      "declarations": ["3:7-3:14|11072669167287398027|2|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["6:24-6:31|0|1|4"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "ns",
      "qual_name_offset": 0,
      "short_name": "ns",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:13|0|1|2",
      "extent": "1:1-9:2|0|1|0",
      "alias_of": 0,
      "bases": [13838176792705659279],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:11-1:13|0|1|4"]
    }, {
      "usr": 13838176792705659279,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [11072669167287398027],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 17262466801709381811,
      "detailed_name": "ns::Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "5:8-5:11|11072669167287398027|2|2",
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
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 17019747379608639279,
      "detailed_name": "void ns::Foo::Register(ns::Manager *m)",
      "qual_name_offset": 5,
      "short_name": "Register",
      "kind": 254,
      "storage": 1,
      "declarations": [],
      "spell": "5:11-5:19|17262466801709381811|2|2",
      "extent": "5:1-6:2|11072669167287398027|2|0",
      "declaring_type": 17262466801709381811,
      "bases": [],
      "derived": [],
      "vars": [13569879755236306838],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 1972401196751872203,
      "detailed_name": "ns::Manager",
      "qual_name_offset": 0,
      "short_name": "Manager",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [13569879755236306838],
      "uses": ["5:20-5:27|0|1|4"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "ns",
      "qual_name_offset": 0,
      "short_name": "ns",
      "kind": 3,
      "declarations": [],
      "spell": "3:11-3:13|0|1|2",
      "extent": "3:1-7:2|0|1|0",
      "alias_of": 0,
      "bases": [13838176792705659279],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["3:11-3:13|0|1|4"]
    }, {
      "usr": 13838176792705659279,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [11072669167287398027],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 17262466801709381811,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [17019747379608639279],
      "vars": [],
      "instances": [],
      "uses": ["5:6-5:9|0|1|4"]
    }],
  "usr2var": [{
      "usr": 13569879755236306838,
      "detailed_name": "ns::Manager *m",
      "qual_name_offset": 13,
      "short_name": "m",
      "declarations": [],
      "spell": "5:29-5:30|17019747379608639279|3|2",
      "extent": "5:20-5:30|17019747379608639279|3|0",
      "type": 1972401196751872203,
      "uses": [],
      "kind": 253,
      "storage": 1
    }]
}
*/