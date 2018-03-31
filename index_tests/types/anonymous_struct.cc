union vector3 {
  struct { float x, y, z; };
  float v[3];
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 17937907487590875128,
      "detailed_name": "vector3",
      "short_name": "vector3",
      "kind": 23,
      "declarations": [],
      "spell": "1:7-1:14|-1|1|2",
      "extent": "1:1-4:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [3],
      "instances": [],
      "uses": []
    }, {
      "id": 1,
      "usr": 1428566502523368801,
      "detailed_name": "vector3::(anon struct)",
      "short_name": "(anon struct)",
      "kind": 23,
      "declarations": [],
      "spell": "2:3-2:9|0|2|2",
      "extent": "2:3-2:28|0|2|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1, 2],
      "instances": [],
      "uses": []
    }, {
      "id": 2,
      "usr": 21,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1, 2, 3],
      "uses": []
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 3348817847649945564,
      "detailed_name": "float vector3::(anon struct)::x",
      "short_name": "x",
      "declarations": [],
      "spell": "2:18-2:19|1|2|2",
      "extent": "2:12-2:19|1|2|0",
      "type": 2,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "id": 1,
      "usr": 4821094820988543895,
      "detailed_name": "float vector3::(anon struct)::y",
      "short_name": "y",
      "declarations": [],
      "spell": "2:21-2:22|1|2|2",
      "extent": "2:12-2:22|1|2|0",
      "type": 2,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "id": 2,
      "usr": 15292551660437765731,
      "detailed_name": "float vector3::(anon struct)::z",
      "short_name": "z",
      "declarations": [],
      "spell": "2:24-2:25|1|2|2",
      "extent": "2:12-2:25|1|2|0",
      "type": 2,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "id": 3,
      "usr": 1963212417280098348,
      "detailed_name": "float [3] vector3::v",
      "short_name": "v",
      "declarations": [],
      "spell": "3:9-3:10|0|2|2",
      "extent": "3:3-3:13|0|2|0",
      "type": 2,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
