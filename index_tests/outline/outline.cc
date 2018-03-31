#include <vector>

struct MergeableUpdate {
  int a;
  int b;
  std::vector<int> to_add;
};

/*
TEXT_REPLACE:
std::__1::vector <===> std::vector
c:@N@std@ST>2#T#T@vector <===> c:@N@std@N@__1@ST>2#T#T@vector
10956461108384510180 <===> 9178760565669096175

OUTPUT:
{
  "includes": [{
      "line": 0,
      "resolved_path": "&vector"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 14399919566014425846,
      "detailed_name": "MergeableUpdate",
      "short_name": "MergeableUpdate",
      "kind": 23,
      "declarations": [],
      "spell": "3:8-3:23|-1|1|2",
      "extent": "3:1-7:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1, 2],
      "instances": [],
      "uses": []
    }, {
      "id": 1,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": []
    }, {
      "id": 2,
      "usr": 9178760565669096175,
      "detailed_name": "std::vector",
      "short_name": "vector",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2],
      "uses": ["6:8-6:14|-1|1|4"]
    }, {
      "id": 3,
      "usr": 5401847601697785946,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["6:3-6:6|0|2|4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 11633578660978286467,
      "detailed_name": "int MergeableUpdate::a",
      "short_name": "a",
      "declarations": [],
      "spell": "4:7-4:8|0|2|2",
      "extent": "4:3-4:8|0|2|0",
      "type": 1,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "id": 1,
      "usr": 14949552147532317793,
      "detailed_name": "int MergeableUpdate::b",
      "short_name": "b",
      "declarations": [],
      "spell": "5:7-5:8|0|2|2",
      "extent": "5:3-5:8|0|2|0",
      "type": 1,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "id": 2,
      "usr": 9003350345237582363,
      "detailed_name": "std::vector<int> MergeableUpdate::to_add",
      "short_name": "to_add",
      "declarations": [],
      "spell": "6:20-6:26|0|2|2",
      "extent": "6:3-6:26|0|2|0",
      "type": 2,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
