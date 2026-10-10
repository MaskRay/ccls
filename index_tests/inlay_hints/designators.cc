struct Inner {
  int x, y;
};
struct Outer {
  Inner a, b;
};
struct Anon {
  union {
    struct {
      int y;
    } x;
  };
};
template <class T, int N> struct Array {
  T __elems[N];
};
struct Base {
  int b;
};
struct Derived : Base {
  int d;
};
struct Ctor {
  Ctor(int x);
};
struct Bits {
  int a;
  int : 3;
  int b;
};
namespace std {
template <class E> class initializer_list {
  const E *b;
  __SIZE_TYPE__ n;
};
} // namespace std
struct List {
  List(std::initializer_list<int> l);
};

Inner i{1, 2};
Outer o{{1, 2}, 3};
Anon an{42};
int arr[] = {1, 2};
Array<int, 2> sa = {1, 2};
Inner sup{/*x=*/1, .y = 2};
Derived der{{1}, 2};
Derived paren({1}, 2);
Inner zero = {};
Ctor ctor{1};
Bits bits{1, 2};
Bits bits_paren(1, 2);
List list{1, 2};
List list2 = {1, 2};

/*
EXTRA_FLAGS:
-std=c++20

OUTPUT:
{
  "inlay_hints": ["41:9|3|.x=", "41:12|3|.y=", "42:9|3|.a=", "42:10|3|.x=", "42:13|3|.y=", "42:17|3|.b.x=", "43:9|3|.x.y=", "44:14|3|[0]=", "44:17|3|[1]=", "45:21|3|[0]=", "45:24|3|[1]=", "47:14|3|.b=", "47:18|3|.d=", "48:16|3|.b=", "48:20|3|.d=", "50:11|2|x:", "51:11|3|.a=", "51:14|3|.b=", "52:17|3|.a=", "52:20|3|.b="]
}
*/
