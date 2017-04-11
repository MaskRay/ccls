#pragma once

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