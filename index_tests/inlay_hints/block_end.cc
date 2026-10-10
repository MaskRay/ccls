namespace ns {
struct S {
  int field;
  bool method() const;
  void out();
  S operator+(S) const;
  explicit operator bool() const;
  enum class E { A };
  union U {};
  //
};

void S::out() {
  for (int i = 0; i < 3; i++) {
    //
    //
    //
    //
    //
    //
    //
    //
  }
  //
}
S S::operator+(S) const {
  //
  //
  //
  //
  //
  //
  //
  //
  return *this;
}

void loops(S s, int n) {
  while (s.method()) {
    //
    //
    //
    //
    //
    //
    //
    //
  }
  if (n > 0) {
    //
    //
    //
    //
  } else if (!s) {
    //
    //
    //
    //
    //
    //
    //
    //
  }
  switch (n) {
  case 0:
    //
    //
    //
    //
    //
    //
    //
    break;
  } // trailing text suppresses the hint
}
} // namespace ns
namespace {
void short_function() {}
}
namespace a::b {
void loop(unsigned u) {
  while (u < 0xffffffffu) {
    //
    //
    //
    //
    //
    //
    //
    //
  }
}
}

/*
OUTPUT:
{
  "inlay_hints": ["11:3|4|// struct S", "23:4|4|// for i", "25:2|4|// S::out", "36:2|4|// S::operator+", "48:4|4|// while method()", "63:4|4|// if", "75:2|4|// loops", "91:4|4|// while u < 4294967295", "92:2|4|// loop", "93:2|4|// namespace a::b"]
}
*/
