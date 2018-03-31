#define A 5
#define DISALLOW(type) type(type&&) = delete;

struct Foo {
  DISALLOW(Foo);
};

int x = A;

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
      "declarations": ["5:12-5:15|-1|1|4"],
      "spell": "4:8-4:11|-1|1|2",
      "extent": "4:1-6:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["5:12-5:15|0|2|4"]
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
      "usr": 13788753348312146871,
      "detailed_name": "void Foo::Foo(Foo &&)",
      "short_name": "Foo",
      "kind": 9,
      "storage": 1,
      "declarations": [],
      "spell": "5:12-5:15|0|2|2",
      "extent": "5:12-5:16|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 10677751717622394455,
      "detailed_name": "int x",
      "short_name": "x",
      "hover": "int x = A",
      "declarations": [],
      "spell": "8:5-8:6|-1|1|2",
      "extent": "8:1-8:10|-1|1|0",
      "type": 1,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 7651988378939587454,
      "detailed_name": "A",
      "short_name": "A",
      "hover": "#define A 5",
      "declarations": [],
      "spell": "1:9-1:10|-1|1|2",
      "extent": "1:9-1:12|-1|1|0",
      "uses": ["8:9-8:10|-1|1|4"],
      "kind": 255,
      "storage": 0
    }, {
      "id": 2,
      "usr": 14946041066794678724,
      "detailed_name": "DISALLOW",
      "short_name": "DISALLOW",
      "hover": "#define DISALLOW(type) type(type&&) = delete;",
      "declarations": [],
      "spell": "2:9-2:17|-1|1|2",
      "extent": "2:9-2:46|-1|1|0",
      "uses": ["5:3-5:11|-1|1|4"],
      "kind": 255,
      "storage": 0
    }]
}
*/
