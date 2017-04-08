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