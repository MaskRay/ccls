enum Foo {
  A,
  B = 20
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 16985894625255407295,
      "detailed_name": "enum Foo {\n}",
      "qual_name_offset": 5,
      "short_name": "Foo",
      "kind": 10,
      "declarations": [],
      "spell": "1:6-1:9|0|1|2",
      "extent": "1:1-4:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 439339022761937396,
      "detailed_name": "A",
      "qual_name_offset": 0,
      "short_name": "A",
      "hover": "A = 0",
      "declarations": [],
      "spell": "2:3-2:4|16985894625255407295|2|514",
      "extent": "2:3-2:4|16985894625255407295|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "usr": 15962370213938840720,
      "detailed_name": "B = 20",
      "qual_name_offset": 0,
      "short_name": "B",
      "declarations": [],
      "spell": "3:3-3:4|16985894625255407295|2|514",
      "extent": "3:3-3:9|16985894625255407295|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }]
}
*/
