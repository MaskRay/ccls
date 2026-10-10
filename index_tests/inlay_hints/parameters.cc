struct S {};
void f(S a, S b);
void g(S &out, const S &in);
void h(S, S);
void h(S x, S y) {}
void setValue(S value);
void setTimeout(S millis);
struct C {
  C(S first, S second);
  void m(S arg);
  void operator()(S arg) const;
};
template <typename... Args> void variadic(Args... args);
template <typename T> struct D {
  void m(S arg);
};
#define CALL f(S(), S())
#define ARG S()

void caller(S a, S other) {
  f(a, other);
  f(/*a=*/other, /* b */ S());
  S s;
  g(s, s);
  h(S(), S());
  setValue(S());
  setTimeout(S());
  C c(a, other);
  C d{a, other};
  S copy(s);
  c.m(S());
  c(S());
  variadic(S(), S());
  CALL;
  f(ARG, ARG);
}

template <typename T> void dependent(D<T> d) { d.m(S()); }

/*
OUTPUT:
{
  "inlay_hints": ["21:8|2|b:", "24:5|2|&out:", "24:8|2|in:", "25:5|2|x:", "25:10|2|y:", "27:14|2|millis:", "28:7|2|first:", "28:10|2|second:", "29:7|2|first:", "29:10|2|second:", "31:7|2|arg:", "32:5|2|arg:", "35:5|2|a:", "35:10|2|b:", "36:2|4|// caller", "38:52|2|arg:"]
}
*/
