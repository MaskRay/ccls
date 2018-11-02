enum class Foo {
  A,
  B = 20
};

Foo x = Foo::A;

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 16985894625255407295,
      "detailed_name": "enum class Foo : int {}",
      "qual_name_offset": 11,
      "short_name": "Foo",
      "spell": "1:12-1:15|1:1-4:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [{
          "L": 439339022761937396,
          "R": -1
        }, {
          "L": 15962370213938840720,
          "R": -1
        }],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [10677751717622394455],
      "uses": ["6:1-6:4|4|-1", "6:9-6:12|4|-1"]
    }],
  "usr2var": [{
      "usr": 439339022761937396,
      "detailed_name": "Foo::A",
      "qual_name_offset": 0,
      "short_name": "A",
      "hover": "Foo::A = 0",
      "spell": "2:3-2:4|2:3-2:4|1026|-1",
      "type": 16985894625255407295,
      "kind": 22,
      "parent_kind": 10,
      "storage": 0,
      "declarations": [],
      "uses": ["6:14-6:15|4|-1"]
    }, {
      "usr": 10677751717622394455,
      "detailed_name": "Foo x",
      "qual_name_offset": 4,
      "short_name": "x",
      "hover": "Foo x = Foo::A",
      "spell": "6:5-6:6|6:1-6:15|2|-1",
      "type": 16985894625255407295,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 15962370213938840720,
      "detailed_name": "Foo::B = 20",
      "qual_name_offset": 0,
      "short_name": "B",
      "spell": "3:3-3:4|3:3-3:9|1026|-1",
      "type": 16985894625255407295,
      "kind": 22,
      "parent_kind": 10,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
