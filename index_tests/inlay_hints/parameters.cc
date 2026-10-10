struct S {};
void f(S left, S right);
void g(S &out, const S &in);
void h(S, S);
void h(S from, S to) {}
void setValue(S value);
void setTimeout(S millis);
struct C {
  C(S first, S second);
  C(S only);
  void m(S arg);
  void operator()(S arg) const;
};
template <typename... Args> void variadic(Args... args);
template <typename T> struct D {
  void m(S arg);
};
#define CALL f(S(), S())
#define ARG S()
enum class Mode { Read };
struct Req {
  S path;
};
struct Ptr {
  S get() const;
};
void open(S path, Mode mode, S flags);
void tiny(S x, S y);
void realPath(S path);
template <typename T> void fwd(T &&ref);

void caller(S left, S other) {
  f(left, other);
  f(/*left=*/other, /* right */ S());
  f(other /*left*/, S());
  S s;
  g(s, s);
  h(S(), S());
  setValue(S());
  setTimeout(S());
  C c(left, other);
  C d{left, other};
  C single(other);
  S copy(s);
  c.m(S());
  c(S());
  variadic(S(), S());
  CALL;
  f(ARG, ARG);
  Req req;
  Ptr path_ptr;
  S cache_path, openFlags;
  open(cache_path, Mode::Read, openFlags);
  open(req.path, Mode::Read, other);
  open(path_ptr.get(), Mode::Read, other);
  tiny(S(), S());
  realPath(other);
  fwd(s);
}

template <typename T> void dependent(D<T> d) { d.m(S()); }

template <class To, class From> To *cast(From *val);
template <class... To, class From> bool isa(const From &val);
template <class T> void convert(T value, int base);
void casts(S *s) {
  cast<S>(s);
  isa<S, C>(*s);
  convert<S>(*s, 10);
}
template <typename T> void dependent_cast(T *p) { cast<S>(p); }

/*
OUTPUT:
{
  "inlay_hints": ["33:11|2|right:", "35:21|2|right:", "37:5|2|&out:", "37:8|2|in:", "38:5|2|from:", "38:10|2|to:", "40:14|2|millis:", "41:7|2|first:", "41:13|2|second:", "42:7|2|first:", "42:13|2|second:", "45:7|2|arg:", "46:5|2|arg:", "49:5|2|left:", "49:10|2|right:", "54:30|2|flags:", "55:36|2|flags:", "58:7|2|ref:", "59:2|4|// caller", "61:52|2|arg:", "69:14|2|value:", "69:18|2|base:"]
}
*/
