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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 6875364467121018690,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "8:6-8:9|0|1|2",
      "extent": "8:1-8:11|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 8905286151237717330,
      "detailed_name": "void C::bar()",
      "qual_name_offset": 5,
      "short_name": "bar",
      "kind": 6,
      "storage": 0,
      "declarations": ["4:8-4:11|8402783583255987702|2|513"],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 8402783583255987702,
      "detailed_name": "struct C {}",
      "qual_name_offset": 7,
      "short_name": "C",
      "kind": 23,
      "declarations": [],
      "spell": "2:8-2:9|0|1|2",
      "extent": "2:1-5:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [8905286151237717330],
      "vars": [{
          "L": 5866801090710377175,
          "R": -1
        }],
      "instances": [],
      "uses": []
    }, {
      "usr": 14750650276757822712,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "1:11-1:18|8402783583255987702|2|2",
      "extent": "1:11-1:18|8402783583255987702|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [5866801090710377175],
      "uses": []
    }],
  "usr2var": [{
      "usr": 5866801090710377175,
      "detailed_name": "T C::x",
      "qual_name_offset": 2,
      "short_name": "x",
      "declarations": [],
      "spell": "3:5-3:6|8402783583255987702|2|514",
      "extent": "3:3-3:6|8402783583255987702|2|0",
      "type": 14750650276757822712,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
