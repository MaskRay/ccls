enum class Foo {
  A,
  B = 20
};

Foo x = Foo::A;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 16985894625255407295,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 10,
      "declarations": [],
      "spell": "1:12-1:15|0|1|2",
      "extent": "1:1-4:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [10677751717622394455],
      "uses": ["6:1-6:4|0|1|4", "6:9-6:12|0|1|4"]
    }],
  "usr2var": [{
      "usr": 439339022761937396,
      "detailed_name": "Foo::A",
      "qual_name_offset": 0,
      "short_name": "A",
      "hover": "Foo::A = 0",
      "declarations": [],
      "spell": "2:3-2:4|16985894625255407295|2|2",
      "extent": "2:3-2:4|16985894625255407295|2|0",
      "type": 16985894625255407295,
      "uses": ["6:14-6:15|0|1|4"],
      "kind": 22,
      "storage": 0
    }, {
      "usr": 10677751717622394455,
      "detailed_name": "Foo x",
      "qual_name_offset": 4,
      "short_name": "x",
      "hover": "Foo x = Foo::A",
      "declarations": [],
      "spell": "6:5-6:6|0|1|2",
      "extent": "6:1-6:15|0|1|0",
      "type": 16985894625255407295,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 15962370213938840720,
      "detailed_name": "Foo::B",
      "qual_name_offset": 0,
      "short_name": "B",
      "hover": "Foo::B = 20",
      "declarations": [],
      "spell": "3:3-3:4|16985894625255407295|2|2",
      "extent": "3:3-3:9|16985894625255407295|2|0",
      "type": 16985894625255407295,
      "uses": [],
      "kind": 22,
      "storage": 0
    }]
}
*/
