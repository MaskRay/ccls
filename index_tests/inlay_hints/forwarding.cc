namespace std {
template <class T> T &&forward(T &t);
}
void *operator new(__SIZE_TYPE__, void *);

struct S {
  S(int a);
  S(int b, int c);
};
template <class T, class... Args> T *make(Args &&...args) { return new T(std::forward<Args>(args)...); }
struct Alloc {
  template <class T, class... Args> void construct(T *p, Args &&...args) {
    ::new ((void *)p) T{std::forward<Args>(args)...};
  }
};
template <class T> struct Vec {
  T *p;
  template <class... Args> void emplace(Args &&...args) { Alloc().construct(p, std::forward<Args>(args)...); }
};
template <class T> struct Base {
  T *p;
  template <class... Args> Base(Args &&...args) : p(new T(std::forward<Args>(args)...)) {}
};
template <class T> struct Derived : Base<T> {
  template <class... Args> Derived(int tag, Args &&...args) : Base<T>(std::forward<Args>(args)...) {}
};
struct Copy {
  Copy();
  Copy(const Copy &other);
};
template <class F, class... Args> void invoke(F f, Args... args) { f(args...); }
template <class... A> struct Gen {
  template <class... B> static void g(A... a, B... b) { S(b...); }
};
void f(int x, int &out);
template <class... Args> void wrap(int head, Args &&...args) { f(std::forward<Args>(args)...); }
template <class... Args> void undefined(Args &&...args);
template <class... Args> void recurse(Args... args);
template <class... Args> void recurse2(Args... args) { recurse(args...); }
template <class... Args> void recurse(Args... args) { recurse2(args...); }

void use(Vec<S> v, int i, Copy cp) {
  make<S>(1);
  make<S>(1, 2);
  v.emplace(1, 2);
  wrap(0, 1, i);
  undefined(i);
  recurse(1, 2);
  Derived<S> d(0, 1, 2);
  invoke([](Copy) {}, cp);
  Gen<int>::g(1, 2, 3);
}

/*
OUTPUT:
{
  "inlay_hints": ["43:11|2|a:", "44:11|2|b:", "44:14|2|c:", "45:13|2|b:", "45:16|2|c:", "46:8|2|head:", "46:11|2|x:", "46:14|2|&out:", "49:16|2|tag:", "49:19|2|b:", "49:22|2|c:", "50:10|2|f:", "50:18|1|-> void", "51:18|2|b:", "51:21|2|c:", "52:2|4|// use"]
}
*/
