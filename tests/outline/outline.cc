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
      "definition_spelling": "3:8-3:23",
      "definition_extent": "3:1-7:2",
      "vars": [0, 1, 2],
      "uses": ["*3:8-3:23"]
    }, {
      "id": 1,
      "usr": "c:@N@std@ST>2#T#T@vector",
      "instantiations": [2],
      "uses": ["*6:8-6:14"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@MergeableUpdate@FI@a",
      "short_name": "a",
      "qualified_name": "MergeableUpdate::a",
      "definition_spelling": "4:7-4:8",
      "definition_extent": "4:3-4:8",
      "declaring_type": 0,
      "uses": ["4:7-4:8"]
    }, {
      "id": 1,
      "usr": "c:@S@MergeableUpdate@FI@b",
      "short_name": "b",
      "qualified_name": "MergeableUpdate::b",
      "definition_spelling": "5:7-5:8",
      "definition_extent": "5:3-5:8",
      "declaring_type": 0,
      "uses": ["5:7-5:8"]
    }, {
      "id": 2,
      "usr": "c:@S@MergeableUpdate@FI@to_add",
      "short_name": "to_add",
      "qualified_name": "MergeableUpdate::to_add",
      "definition_spelling": "6:20-6:26",
      "definition_extent": "6:3-6:26",
      "variable_type": 1,
      "declaring_type": 0,
      "uses": ["6:20-6:26"]
    }]
}
*/
