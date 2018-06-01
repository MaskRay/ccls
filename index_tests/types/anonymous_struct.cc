union vector3 {
  struct { float x, y, z; };
  float v[3];
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 21,
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
      "instances": [3348817847649945564, 4821094820988543895, 15292551660437765731, 1963212417280098348],
      "uses": []
    }, {
      "usr": 1428566502523368801,
      "detailed_name": "vector3::(anon struct)",
      "qual_name_offset": 0,
      "short_name": "(anon struct)",
      "kind": 23,
      "declarations": [],
      "spell": "2:3-2:9|17937907487590875128|2|2",
      "extent": "2:3-2:28|17937907487590875128|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [{
          "L": 3348817847649945564,
          "R": 0
        }, {
          "L": 4821094820988543895,
          "R": 32
        }, {
          "L": 15292551660437765731,
          "R": 64
        }],
      "instances": [],
      "uses": []
    }, {
      "usr": 17937907487590875128,
      "detailed_name": "vector3",
      "qual_name_offset": 0,
      "short_name": "vector3",
      "kind": 23,
      "declarations": [],
      "spell": "1:7-1:14|0|1|2",
      "extent": "1:1-4:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [{
          "L": 1963212417280098348,
          "R": 0
        }],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 1963212417280098348,
      "detailed_name": "float [3] vector3::v",
      "qual_name_offset": 10,
      "short_name": "v",
      "hover": "float [3] vector3::v[3]",
      "declarations": [],
      "spell": "3:9-3:10|17937907487590875128|2|2",
      "extent": "3:3-3:13|17937907487590875128|2|0",
      "type": 21,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "usr": 3348817847649945564,
      "detailed_name": "float vector3::(anon struct)::x",
      "qual_name_offset": 6,
      "short_name": "x",
      "declarations": [],
      "spell": "2:18-2:19|1428566502523368801|2|2",
      "extent": "2:12-2:19|1428566502523368801|2|0",
      "type": 21,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "usr": 4821094820988543895,
      "detailed_name": "float vector3::(anon struct)::y",
      "qual_name_offset": 6,
      "short_name": "y",
      "declarations": [],
      "spell": "2:21-2:22|1428566502523368801|2|2",
      "extent": "2:12-2:22|1428566502523368801|2|0",
      "type": 21,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "usr": 15292551660437765731,
      "detailed_name": "float vector3::(anon struct)::z",
      "qual_name_offset": 6,
      "short_name": "z",
      "declarations": [],
      "spell": "2:24-2:25|1428566502523368801|2|2",
      "extent": "2:12-2:25|1428566502523368801|2|0",
      "type": 21,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
