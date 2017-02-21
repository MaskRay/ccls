struct T {};

extern T t;
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@T",
      "short_name": "T",
      "qualified_name": "T",
      "definition": "1:1:8",
      "uses": ["*1:1:8", "*1:3:8"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@t",
      "short_name": "t",
      "qualified_name": "t",
      "declaration": "1:3:10",
      "variable_type": 0,
      "uses": ["1:3:10"]
    }]
}
*/