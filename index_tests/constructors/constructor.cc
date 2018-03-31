class Foo {
public:
  Foo() {}
};

void foo() {
  Foo f;
  Foo* f2 = new Foo();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 5,
      "declarations": ["3:3-3:6|-1|1|4"],
      "spell": "1:7-1:10|-1|1|2",
      "extent": "1:1-4:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [0, 1],
      "uses": ["3:3-3:6|0|2|4", "7:3-7:6|-1|1|4", "8:3-8:6|-1|1|4", "8:17-8:20|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 3385168158331140247,
      "detailed_name": "void Foo::Foo()",
      "short_name": "Foo",
      "kind": 9,
      "storage": 1,
      "declarations": [],
      "spell": "3:3-3:6|0|2|2",
      "extent": "3:3-3:11|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["7:7-7:8|1|3|288", "8:17-8:20|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "6:6-6:9|-1|1|2",
      "extent": "6:1-9:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": ["7:7-7:8|0|3|288", "7:7-7:8|0|3|288", "8:17-8:20|0|3|32", "8:17-8:20|0|3|32"]
    }],
  "vars": [{
      "id": 0,
      "usr": 18410644574635149442,
      "detailed_name": "Foo f",
      "short_name": "f",
      "declarations": [],
      "spell": "7:7-7:8|1|3|2",
      "extent": "7:3-7:8|1|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 11468802633764653592,
      "detailed_name": "Foo *f2",
      "short_name": "f2",
      "hover": "Foo *f2 = new Foo()",
      "declarations": [],
      "spell": "8:8-8:10|1|3|2",
      "extent": "8:3-8:22|1|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
