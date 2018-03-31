struct Foo {
  static int x;
};

void accept(int);

void foo() {
  accept(Foo::x);
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
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["8:10-8:13|-1|1|4"]
    }, {
      "id": 1,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 17175780305784503374,
      "detailed_name": "void accept(int)",
      "short_name": "accept",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "5:6-5:12|-1|1|1",
          "param_spellings": ["5:16-5:16"]
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:3-8:9|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "7:6-7:9|-1|1|2",
      "extent": "7:1-9:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["8:3-8:9|0|3|32"]
    }],
  "vars": [{
      "id": 0,
      "usr": 8599782646965457351,
      "detailed_name": "int Foo::x",
      "short_name": "x",
      "declarations": ["2:14-2:15|0|2|1"],
      "type": 1,
      "uses": ["8:15-8:16|1|3|4"],
      "kind": 8,
      "storage": 3
    }]
}
*/
