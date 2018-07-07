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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "10:6-10:9|0|1|2",
      "extent": "10:1-18:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 12086644540399881766,
      "detailed_name": "void accept(int *)",
      "qual_name_offset": 5,
      "short_name": "accept",
      "kind": 12,
      "storage": 0,
      "declarations": ["8:6-8:12|0|1|1"],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["16:3-16:9|0|1|8228"],
      "callees": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [4220150017963593039, 3873837747174060388],
      "uses": [],
      "callees": []
    }, {
      "usr": 17175780305784503374,
      "detailed_name": "void accept(int)",
      "qual_name_offset": 5,
      "short_name": "accept",
      "kind": 12,
      "storage": 0,
      "declarations": ["7:6-7:12|0|1|1"],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["14:3-14:9|0|1|8228", "15:3-15:9|0|1|8228", "17:3-17:9|0|1|8228"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [4220150017963593039, 3873837747174060388],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-5:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [{
          "L": 4220150017963593039,
          "R": 0
        }, {
          "L": 3873837747174060388,
          "R": 32
        }],
      "instances": [14669930844300034456],
      "uses": ["11:3-11:6|0|1|4"]
    }],
  "usr2var": [{
      "usr": 3873837747174060388,
      "detailed_name": "int Foo::y",
      "qual_name_offset": 4,
      "short_name": "y",
      "declarations": [],
      "spell": "4:7-4:8|15041163540773201510|2|514",
      "extent": "4:3-4:8|15041163540773201510|2|0",
      "type": 53,
      "uses": ["17:12-17:13|15041163540773201510|2|12"],
      "kind": 8,
      "storage": 0
    }, {
      "usr": 4220150017963593039,
      "detailed_name": "int Foo::x",
      "qual_name_offset": 4,
      "short_name": "x",
      "declarations": [],
      "spell": "3:7-3:8|15041163540773201510|2|514",
      "extent": "3:3-3:8|15041163540773201510|2|0",
      "type": 53,
      "uses": ["12:5-12:6|15041163540773201510|2|20", "13:5-13:6|15041163540773201510|2|4", "14:12-14:13|15041163540773201510|2|12", "15:12-15:13|15041163540773201510|2|12", "16:13-16:14|15041163540773201510|2|132"],
      "kind": 8,
      "storage": 0
    }, {
      "usr": 14669930844300034456,
      "detailed_name": "Foo f",
      "qual_name_offset": 4,
      "short_name": "f",
      "declarations": [],
      "spell": "11:7-11:8|4259594751088586730|3|2",
      "extent": "11:3-11:8|4259594751088586730|3|0",
      "type": 15041163540773201510,
      "uses": ["12:3-12:4|4259594751088586730|3|4", "13:3-13:4|4259594751088586730|3|4", "14:10-14:11|4259594751088586730|3|4", "15:10-15:11|4259594751088586730|3|4", "16:11-16:12|4259594751088586730|3|4", "17:10-17:11|4259594751088586730|3|4"],
      "kind": 13,
      "storage": 0
    }]
}
*/
