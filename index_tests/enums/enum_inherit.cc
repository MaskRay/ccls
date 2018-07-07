enum Foo : int {
  A,
  B = 20
};

typedef int int32_t;

enum class E : int32_t {
  E0,
  E20 = 20
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 2986879766914123941,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [16614320383091394267, 16847439761518576294],
      "uses": [],
      "callees": []
    }, {
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
      "usr": 2986879766914123941,
      "detailed_name": "enum class E : int32_t {\n}",
      "qual_name_offset": 11,
      "short_name": "E",
      "kind": 10,
      "declarations": [],
      "spell": "8:12-8:13|0|1|2",
      "extent": "8:1-11:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 14939241684006947339,
      "detailed_name": "typedef int int32_t",
      "qual_name_offset": 12,
      "short_name": "int32_t",
      "kind": 252,
      "declarations": [],
      "spell": "6:13-6:20|0|1|2",
      "extent": "6:1-6:20|0|1|0",
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
      "detailed_name": "enum Foo : int {\n}",
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
      "hover": "B = 20 = 20",
      "declarations": [],
      "spell": "3:3-3:4|16985894625255407295|2|514",
      "extent": "3:3-3:9|16985894625255407295|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "usr": 16614320383091394267,
      "detailed_name": "E::E0",
      "qual_name_offset": 0,
      "short_name": "E0",
      "hover": "E::E0 = 0",
      "declarations": [],
      "spell": "9:3-9:5|2986879766914123941|2|514",
      "extent": "9:3-9:5|2986879766914123941|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "usr": 16847439761518576294,
      "detailed_name": "E::E20 = 20",
      "qual_name_offset": 0,
      "short_name": "E20",
      "hover": "E::E20 = 20 = 20",
      "declarations": [],
      "spell": "10:3-10:6|2986879766914123941|2|514",
      "extent": "10:3-10:11|2986879766914123941|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }]
}
*/
