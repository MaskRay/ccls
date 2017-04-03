#include <vector>

struct MergeableUpdate {
  int a;
  int b;
  std::vector<int> to_add;
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@MergeableUpdate",
      "short_name": "MergeableUpdate",
      "qualified_name": "MergeableUpdate",
      "definition": "1:3:8",
      "vars": [0, 1, 2],
      "uses": ["*1:3:8"]
    }, {
      "id": 1,
      "usr": "c:@N@std@ST>2#T#T@vector",
      "instantiations": [2],
      "uses": ["*1:6:8"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@MergeableUpdate@FI@a",
      "short_name": "a",
      "qualified_name": "MergeableUpdate::a",
      "definition": "1:4:7",
      "declaring_type": 0,
      "uses": ["1:4:7"]
    }, {
      "id": 1,
      "usr": "c:@S@MergeableUpdate@FI@b",
      "short_name": "b",
      "qualified_name": "MergeableUpdate::b",
      "definition": "1:5:7",
      "declaring_type": 0,
      "uses": ["1:5:7"]
    }, {
      "id": 2,
      "usr": "c:@S@MergeableUpdate@FI@to_add",
      "short_name": "to_add",
      "qualified_name": "MergeableUpdate::to_add",
      "definition": "1:6:20",
      "variable_type": 1,
      "declaring_type": 0,
      "uses": ["1:6:20"]
    }]
}
*/
