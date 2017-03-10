#include <vector>

struct MergeableUpdate {
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
      "definition": "1:1:8",
      "uses": ["*1:1:8"]
    }]
}
*/
