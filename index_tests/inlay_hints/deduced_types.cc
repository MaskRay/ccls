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

template <typename T> void one(T t) {
  auto local = t;
}
template <typename T> void two(T t) {
  auto local = t;
}
void abbreviated(auto x) {}

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
  "inlay_hints": ["7:7|1|: S", "8:8|1|: S &", "9:8|1|: S", "9:11|1|: S", "10:11|1|-> S", "11:12|1|: S", "14:13|1|: S", "17:13|1|: T", "19:24|1|: S", "22:7|2|t:", "23:7|2|t:", "24:7|2|t:", "25:15|2|x:", "26:24|1|-> S"]
}
*/
