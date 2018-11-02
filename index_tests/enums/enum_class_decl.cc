typedef unsigned char uint8_t;
enum class Foo : uint8_t {
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
      "usr": 2010430204259339553,
      "detailed_name": "typedef unsigned char uint8_t",
      "qual_name_offset": 22,
      "short_name": "uint8_t",
      "spell": "1:23-1:30|1:1-1:30|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 252,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 16985894625255407295,
      "detailed_name": "enum class Foo : uint8_t {}",
      "qual_name_offset": 11,
      "short_name": "Foo",
      "spell": "2:12-2:15|2:1-5:2|2|-1",
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
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 439339022761937396,
      "detailed_name": "Foo::A",
      "qual_name_offset": 0,
      "short_name": "A",
      "hover": "Foo::A = 0",
      "spell": "3:3-3:4|3:3-3:4|1026|-1",
      "type": 16985894625255407295,
      "kind": 22,
      "parent_kind": 10,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 15962370213938840720,
      "detailed_name": "Foo::B = 20",
      "qual_name_offset": 0,
      "short_name": "B",
      "spell": "4:3-4:4|4:3-4:9|1026|-1",
      "type": 16985894625255407295,
      "kind": 22,
      "parent_kind": 10,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
