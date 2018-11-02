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
      "detailed_name": "enum Foo {}",
      "qual_name_offset": 5,
      "short_name": "Foo",
      "spell": "1:6-1:9|1:1-4:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 439339022761937396,
      "detailed_name": "A",
      "qual_name_offset": 0,
      "short_name": "A",
      "hover": "A = 0",
      "spell": "2:3-2:4|2:3-2:4|1026|-1",
      "type": 16985894625255407295,
      "kind": 22,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 15962370213938840720,
      "detailed_name": "B = 20",
      "qual_name_offset": 0,
      "short_name": "B",
      "spell": "3:3-3:4|3:3-3:9|1026|-1",
      "type": 16985894625255407295,
      "kind": 22,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
