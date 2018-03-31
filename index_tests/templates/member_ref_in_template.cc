template <class T>
struct C {
  T x;
  void bar();
};

template <class T>
void foo() {
  C<T> d;
  d.x;  // spelling range is empty, use cursor extent for range
  d.bar();  // spelling range is empty, use cursor extent for range

  auto e = new C<T>;
  e->x;  // `x` seems not exposed by libclang
  e->bar();  // `bar` seems not exposed by libclang
}

/*
EXTRA_FLAGS:
-fms-extensions
-fms-compatibility
-fdelayed-template-parsing

OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 8402783583255987702,
      "detailed_name": "C",
      "short_name": "C",
      "kind": 5,
      "declarations": [],
      "spell": "2:8-2:9|-1|1|2",
      "extent": "2:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [0],
      "instances": [],
      "uses": []
    }, {
      "id": 1,
      "usr": 14750650276757822712,
      "detailed_name": "T",
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "1:17-1:18|-1|1|2",
      "extent": "1:11-1:18|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["3:3-3:4|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 8905286151237717330,
      "detailed_name": "void C::bar()",
      "short_name": "bar",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "4:8-4:11|0|2|1",
          "param_spellings": []
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 1,
      "usr": 6875364467121018690,
      "detailed_name": "void foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "8:6-8:9|-1|1|2",
      "extent": "8:1-8:11|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 5866801090710377175,
      "detailed_name": "T C::x",
      "short_name": "x",
      "declarations": [],
      "spell": "3:5-3:6|0|2|2",
      "extent": "3:3-3:6|0|2|0",
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
