class Foo {
public:
  int x;
  int y;
};

void accept(int);
void accept(int*);

void foo() {
  Foo f;
  f.x = 3;
  f.x += 5;
  accept(f.x);
  accept(f.x + 20);
  accept(&f.x);
  accept(f.y);
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
      "declarations": [],
      "spell": "1:7-1:10|-1|1|2",
      "extent": "1:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1],
      "instances": [2],
      "uses": ["11:3-11:6|-1|1|4"]
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
      "instances": [0, 1],
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
          "spell": "7:6-7:12|-1|1|1",
          "param_spellings": ["7:16-7:16"]
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["14:3-14:9|2|3|32", "15:3-15:9|2|3|32", "17:3-17:9|2|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 12086644540399881766,
      "detailed_name": "void accept(int *)",
      "short_name": "accept",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "8:6-8:12|-1|1|1",
          "param_spellings": ["8:17-8:17"]
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["16:3-16:9|2|3|32"],
      "callees": []
    }, {
      "id": 2,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "10:6-10:9|-1|1|2",
      "extent": "10:1-18:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [2],
      "uses": [],
      "callees": ["14:3-14:9|0|3|32", "15:3-15:9|0|3|32", "16:3-16:9|1|3|32", "17:3-17:9|0|3|32"]
    }],
  "vars": [{
      "id": 0,
      "usr": 4220150017963593039,
      "detailed_name": "int Foo::x",
      "short_name": "x",
      "declarations": [],
      "spell": "3:7-3:8|0|2|2",
      "extent": "3:3-3:8|0|2|0",
      "type": 1,
      "uses": ["12:5-12:6|2|3|4", "13:5-13:6|2|3|4", "14:12-14:13|2|3|4", "15:12-15:13|2|3|4", "16:13-16:14|2|3|4"],
      "kind": 8,
      "storage": 0
    }, {
      "id": 1,
      "usr": 3873837747174060388,
      "detailed_name": "int Foo::y",
      "short_name": "y",
      "declarations": [],
      "spell": "4:7-4:8|0|2|2",
      "extent": "4:3-4:8|0|2|0",
      "type": 1,
      "uses": ["17:12-17:13|2|3|4"],
      "kind": 8,
      "storage": 0
    }, {
      "id": 2,
      "usr": 16303259148898744165,
      "detailed_name": "Foo f",
      "short_name": "f",
      "declarations": [],
      "spell": "11:7-11:8|2|3|2",
      "extent": "11:3-11:8|2|3|0",
      "type": 0,
      "uses": ["12:3-12:4|2|3|4", "13:3-13:4|2|3|4", "14:10-14:11|2|3|4", "15:10-15:11|2|3|4", "16:11-16:12|2|3|4", "17:10-17:11|2|3|4"],
      "kind": 13,
      "storage": 1
    }]
}
*/
