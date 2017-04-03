#pragma once

#include "../../third_party/doctest/doctest/doctest.h"
#include "../../third_party/macro_map.h"
#include "../../third_party/optional.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

struct Base {};

struct SameFileDerived : Base {};

using Foo0 = SameFileDerived;

template <typename T>
void Foo1() {}

template <typename T>
struct Foo2 {};

enum Foo3 { A, B, C };

int Foo4;
static int Foo5;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Base",
      "short_name": "Base",
      "qualified_name": "Base",
      "definition": "1:10:8",
      "derived": [1],
      "uses": ["*1:10:8", "*1:12:26"]
    }, {
      "id": 1,
      "usr": "c:@S@SameFileDerived",
      "short_name": "SameFileDerived",
      "qualified_name": "SameFileDerived",
      "definition": "1:12:8",
      "parents": [0],
      "uses": ["*1:12:8", "*1:14:14"]
    }, {
      "id": 2,
      "usr": "c:@Foo0",
      "short_name": "Foo0",
      "qualified_name": "Foo0",
      "definition": "1:14:7",
      "alias_of": 1,
      "uses": ["*1:14:7"]
    }, {
      "id": 3,
      "usr": "c:@ST>1#T@Foo2",
      "short_name": "Foo2",
      "qualified_name": "Foo2",
      "definition": "1:20:8",
      "uses": ["*1:20:8"]
    }, {
      "id": 4,
      "usr": "c:@E@Foo3",
      "short_name": "Foo3",
      "qualified_name": "Foo3",
      "definition": "1:22:6",
      "vars": [0, 1, 2],
      "uses": ["*1:22:6"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@FT@>1#TFoo1#v#",
      "short_name": "Foo1",
      "qualified_name": "Foo1",
      "definition": "1:17:6",
      "uses": ["1:17:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@E@Foo3@A",
      "short_name": "A",
      "qualified_name": "Foo3::A",
      "definition": "1:22:13",
      "variable_type": 4,
      "declaring_type": 4,
      "uses": ["1:22:13"]
    }, {
      "id": 1,
      "usr": "c:@E@Foo3@B",
      "short_name": "B",
      "qualified_name": "Foo3::B",
      "definition": "1:22:16",
      "variable_type": 4,
      "declaring_type": 4,
      "uses": ["1:22:16"]
    }, {
      "id": 2,
      "usr": "c:@E@Foo3@C",
      "short_name": "C",
      "qualified_name": "Foo3::C",
      "definition": "1:22:19",
      "variable_type": 4,
      "declaring_type": 4,
      "uses": ["1:22:19"]
    }, {
      "id": 3,
      "usr": "c:@Foo4",
      "short_name": "Foo4",
      "qualified_name": "Foo4",
      "definition": "1:24:5",
      "uses": ["1:24:5"]
    }, {
      "id": 4,
      "usr": "c:header.h@Foo5",
      "short_name": "Foo5",
      "qualified_name": "Foo5",
      "definition": "1:25:12",
      "uses": ["1:25:12"]
    }]
}
*/
