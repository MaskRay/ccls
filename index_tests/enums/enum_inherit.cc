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
  "usr2func": [],
  "usr2type": [{
      "usr": 2986879766914123941,
      "detailed_name": "enum class E : int32_t {}",
      "qual_name_offset": 11,
      "short_name": "E",
      "spell": "8:12-8:13|8:1-11:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [{
          "L": 16614320383091394267,
          "R": -1
        }, {
          "L": 16847439761518576294,
          "R": -1
        }],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 14939241684006947339,
      "detailed_name": "typedef int int32_t",
      "qual_name_offset": 12,
      "short_name": "int32_t",
      "spell": "6:13-6:20|6:1-6:20|2|-1",
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
      "detailed_name": "enum Foo : int {}",
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
    }, {
      "usr": 16614320383091394267,
      "detailed_name": "E::E0",
      "qual_name_offset": 0,
      "short_name": "E0",
      "hover": "E::E0 = 0",
      "spell": "9:3-9:5|9:3-9:5|1026|-1",
      "type": 2986879766914123941,
      "kind": 22,
      "parent_kind": 10,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16847439761518576294,
      "detailed_name": "E::E20 = 20",
      "qual_name_offset": 0,
      "short_name": "E20",
      "spell": "10:3-10:6|10:3-10:11|1026|-1",
      "type": 2986879766914123941,
      "kind": 22,
      "parent_kind": 10,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
