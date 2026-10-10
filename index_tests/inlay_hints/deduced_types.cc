struct S {};
struct Pair {
  S first;
  S second;
};
S make();
auto v = make();
auto &r = v;
auto [p, q] = Pair();
auto ret() { return S(); }
decltype(v) w;
template <class T> struct Box {};
template <class T> using __box_t = Box<T>;
__box_t<S> make_box();
auto box = make_box();
template <class T> auto deduce(T t) { return t; }
template <class T> void undeduced(T t) { auto it = deduce(t); }

template <typename T> void one(T arg) {
  auto local = arg;
}
template <typename T> void two(T arg) {
  auto local = arg;
}
void abbreviated(auto value) {}

void use() {
  one(S());
  two(S());
  two(Pair());
  abbreviated(S());
  auto lambda = [](S s) { return s; };
  auto &lambda_ref = lambda;
}

/*
OUTPUT:
{
  "inlay_hints": ["7:7|1|: S", "8:8|1|: S &", "9:8|1|: S", "9:11|1|: S", "10:11|1|-> S", "11:12|1|: S", "15:9|1|: Box<S>", "20:13|1|: S", "23:13|1|: T", "25:28|1|: S", "28:7|2|arg:", "29:7|2|arg:", "30:7|2|arg:", "31:15|2|value:", "32:24|1|-> S"]
}
*/
