#if false
/*
abc
daaa
faf
dakkdakk
abaa
*/
#include <string>

#include "a.h"


struct iface {
  virtual void foo() = 0;
};
struct impl : public iface {
  void foo() override {}
};

void doit() {
  iface* f;
  f->foo();
}

struct Middle : public Parent {
  void foo() override {}
};
struct DerivedA : public Middle {
  void foo() override {}
};
struct DerivedB : public Middle {
  void foo() override {}
};
struct Derived2B : public DerivedB {
  void foo() override {}
};

struct Derived2C : public DerivedB {
  void foo() override;
};


void Derived2C::foo() {}

void User() {
  Parent p;
  Middle m;
  DerivedA da;
  DerivedB db;
  Derived2B d2b;

  p.foo();
  m.foo();
  da.foo();
  db.foo();
  d2b.foo();
}





struct Saaaaaa {};

struct S2 {
  S2() {}

  int a;
  int b;
};

struct MyFoo {
  std::string name;
};

void f() {
  S2 s2;
  s2.a += 10;
  s2.b -= 100;
  s2.b -= 5;

  MyFoo f;
  // f.name = 10;
  f.name = "okay";

  MyFoo f2;
  f2.name = "yes!";
}


void baz();

void foo();
void foo();

void foo() {}

/**/
void caller() {
  MyFoo fff;
  fff.name = "this name";
  baz();
  baz();
  baz();
  foo();

  foo();
  foo();
}
#endif