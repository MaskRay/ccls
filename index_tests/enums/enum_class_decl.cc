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
  "usr2func": [{
      "usr": 16985894625255407295,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [439339022761937396, 15962370213938840720],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 2010430204259339553,
      "detailed_name": "typedef unsigned char uint8_t",
      "qual_name_offset": 22,
      "short_name": "uint8_t",
      "kind": 252,
      "declarations": [],
      "spell": "1:23-1:30|0|1|2",
      "extent": "1:1-1:30|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 16985894625255407295,
      "detailed_name": "enum class Foo : uint8_t {\n}",
      "qual_name_offset": 11,
      "short_name": "Foo",
      "kind": 10,
      "declarations": [],
      "spell": "2:12-2:15|0|1|2",
      "extent": "2:1-5:2|0|1|0",
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
      "detailed_name": "Foo::A",
      "qual_name_offset": 0,
      "short_name": "A",
      "hover": "Foo::A = 0",
      "declarations": [],
      "spell": "3:3-3:4|16985894625255407295|2|514",
      "extent": "3:3-3:4|16985894625255407295|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "usr": 15962370213938840720,
      "detailed_name": "Foo::B = 20",
      "qual_name_offset": 0,
      "short_name": "B",
      "declarations": [],
      "spell": "4:3-4:4|16985894625255407295|2|514",
      "extent": "4:3-4:9|16985894625255407295|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }]
}
*/
