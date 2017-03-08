union vector3 {
  struct { float x, y, z; };
  float v[3];
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@U@vector3",
      "short_name": "vector3",
      "qualified_name": "vector3",
      "definition": "1:1:7",
      "vars": [3],
      "uses": ["*1:1:7"]
    }, {
      "id": 1,
      "usr": "c:@U@vector3@Sa",
      "short_name": "<anonymous>",
      "qualified_name": "vector3::<anonymous>",
      "definition": "1:2:3",
      "vars": [0, 1, 2],
      "uses": ["*1:2:3"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@U@vector3@Sa@FI@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition": "1:2:18",
      "declaring_type": 1,
      "uses": ["1:2:18"]
    }, {
      "id": 1,
      "usr": "c:@U@vector3@Sa@FI@y",
      "short_name": "y",
      "qualified_name": "y",
      "definition": "1:2:21",
      "declaring_type": 1,
      "uses": ["1:2:21"]
    }, {
      "id": 2,
      "usr": "c:@U@vector3@Sa@FI@z",
      "short_name": "z",
      "qualified_name": "z",
      "definition": "1:2:24",
      "declaring_type": 1,
      "uses": ["1:2:24"]
    }, {
      "id": 3,
      "usr": "c:@U@vector3@FI@v",
      "short_name": "v",
      "qualified_name": "vector3::v",
      "definition": "1:3:9",
      "declaring_type": 0,
      "uses": ["1:3:9"]
    }]
}
*/
