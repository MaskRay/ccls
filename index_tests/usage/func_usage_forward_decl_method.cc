struct Foo {
  void foo();
};

void usage() {
  Foo* f = nullptr;
  f->foo();
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
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:11|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [0],
      "uses": ["6:3-6:6|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 17922201480358737771,
      "detailed_name": "void Foo::foo()",
      "short_name": "foo",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "2:8-2:11|0|2|1",
          "param_spellings": []
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["7:6-7:9|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 6767773193109753523,
      "detailed_name": "void usage()",
      "short_name": "usage",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "5:6-5:11|-1|1|2",
      "extent": "5:1-8:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0],
      "uses": [],
      "callees": ["7:6-7:9|0|3|32"]
    }],
  "vars": [{
      "id": 0,
      "usr": 12410753116854389823,
      "detailed_name": "Foo *f",
      "short_name": "f",
      "hover": "Foo *f = nullptr",
      "declarations": [],
      "spell": "6:8-6:9|1|3|2",
      "extent": "6:3-6:19|1|3|0",
      "type": 0,
      "uses": ["7:3-7:4|1|3|4"],
      "kind": 13,
      "storage": 1
    }]
}
*/
