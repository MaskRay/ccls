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
  "types": [{
      "id": 0,
      "usr": 11072669167287398027,
      "detailed_name": "ns",
      "short_name": "ns",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:13|-1|1|2",
      "extent": "1:1-9:2|-1|1|0",
      "bases": [1],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:11-1:13|-1|1|4"]
    }, {
      "id": 1,
      "usr": 13838176792705659279,
      "detailed_name": "<fundamental>",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [0],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 2,
      "usr": 1972401196751872203,
      "detailed_name": "ns::Manager",
      "short_name": "Manager",
      "kind": 5,
      "declarations": ["3:7-3:14|0|2|1"],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["6:24-6:31|-1|1|4"]
    }, {
      "id": 3,
      "usr": 17262466801709381811,
      "detailed_name": "ns::Foo",
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "5:8-5:11|0|2|2",
      "extent": "5:1-7:2|0|2|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 17019747379608639279,
      "detailed_name": "void ns::Foo::Register(ns::Manager *)",
      "short_name": "Register",
      "kind": 254,
      "storage": 3,
      "declarations": [{
          "spell": "6:15-6:23|3|2|1",
          "param_spellings": ["6:32-6:32"]
        }],
      "declaring_type": 3,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
OUTPUT: static_function_in_type.cc
{
  "includes": [{
      "line": 0,
      "resolved_path": "&static_function_in_type.h"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 11072669167287398027,
      "detailed_name": "ns",
      "short_name": "ns",
      "kind": 3,
      "declarations": [],
      "spell": "3:11-3:13|-1|1|2",
      "extent": "3:1-7:2|-1|1|0",
      "bases": [1],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["3:11-3:13|-1|1|4"]
    }, {
      "id": 1,
      "usr": 13838176792705659279,
      "detailed_name": "<fundamental>",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [0],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 2,
      "usr": 17262466801709381811,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["5:6-5:9|-1|1|4"]
    }, {
      "id": 3,
      "usr": 1972401196751872203,
      "detailed_name": "ns::Manager",
      "short_name": "Manager",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["5:20-5:27|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 17019747379608639279,
      "detailed_name": "void ns::Foo::Register(ns::Manager *m)",
      "short_name": "Register",
      "kind": 254,
      "storage": 1,
      "declarations": [],
      "spell": "5:11-5:19|2|2|2",
      "extent": "5:1-6:2|0|2|0",
      "declaring_type": 2,
      "bases": [],
      "derived": [],
      "vars": [0],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 9285345059965948351,
      "detailed_name": "ns::Manager *m",
      "short_name": "m",
      "declarations": [],
      "spell": "5:29-5:30|0|3|2",
      "extent": "5:20-5:30|0|3|0",
      "type": 3,
      "uses": [],
      "kind": 253,
      "storage": 1
    }]
}
*/