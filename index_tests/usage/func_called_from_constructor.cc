void called() {}

struct Foo {
  Foo();
};

Foo::Foo() {
  called();
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
      "declarations": ["4:3-4:6|-1|1|4", "7:6-7:9|-1|1|4"],
      "spell": "3:8-3:11|-1|1|2",
      "extent": "3:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [1],
      "vars": [],
      "instances": [],
      "uses": ["4:3-4:6|0|2|4", "7:6-7:9|-1|1|4", "7:1-7:4|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 468307235068920063,
      "detailed_name": "void called()",
      "short_name": "called",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "1:6-1:12|-1|1|2",
      "extent": "1:1-1:17|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:3-8:9|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 3385168158331140247,
      "detailed_name": "void Foo::Foo()",
      "short_name": "Foo",
      "kind": 9,
      "storage": 1,
      "declarations": [{
          "spell": "4:3-4:6|0|2|1",
          "param_spellings": []
        }],
      "spell": "7:6-7:9|0|2|2",
      "extent": "7:1-9:2|-1|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["8:3-8:9|0|3|32"]
    }],
  "vars": []
}
*/
