void consume(void (*)()) {}

void used() {}

void user() {
  void (*x)() = &used;
  consume(&used);
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "usr": 12924914488846929470,
      "detailed_name": "void consume(void (*)())",
      "short_name": "consume",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "1:6-1:13|-1|1|2",
      "extent": "1:1-1:28|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["7:3-7:10|2|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 5264867802674151787,
      "detailed_name": "void used()",
      "short_name": "used",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:6-3:10|-1|1|2",
      "extent": "3:1-3:15|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:18-6:22|2|3|32", "7:12-7:16|2|3|32"],
      "callees": []
    }, {
      "id": 2,
      "usr": 9376923949268137283,
      "detailed_name": "void user()",
      "short_name": "user",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "5:6-5:10|-1|1|2",
      "extent": "5:1-8:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0],
      "uses": [],
      "callees": ["6:18-6:22|1|3|32", "6:18-6:22|1|3|32", "7:3-7:10|0|3|32", "7:12-7:16|1|3|32"]
    }],
  "vars": [{
      "id": 0,
      "usr": 13681544683892648258,
      "detailed_name": "void (*)() x",
      "short_name": "x",
      "declarations": [],
      "spell": "6:10-6:11|2|3|2",
      "extent": "6:3-6:22|2|3|0",
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
