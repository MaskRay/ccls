struct Type {};

void foo(Type& a0, const Type& a1) {
  Type a2;
  Type* a3;
  const Type* a4;
  const Type* const a5 = nullptr;
}
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 13487927231218873822,
      "detailed_name": "Type",
      "short_name": "Type",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:12|-1|1|2",
      "extent": "1:1-1:15|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1, 2, 3, 4, 5],
      "uses": ["3:10-3:14|-1|1|4", "3:26-3:30|-1|1|4", "4:3-4:7|-1|1|4", "5:3-5:7|-1|1|4", "6:9-6:13|-1|1|4", "7:9-7:13|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 16858540520096802573,
      "detailed_name": "void foo(Type &a0, const Type &a1)",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:6-3:9|-1|1|2",
      "extent": "3:1-8:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1, 2, 3, 4, 5],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 16414210592877294238,
      "detailed_name": "Type &a0",
      "short_name": "a0",
      "declarations": [],
      "spell": "3:16-3:18|0|3|2",
      "extent": "3:10-3:18|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 1
    }, {
      "id": 1,
      "usr": 11558141642862804306,
      "detailed_name": "const Type &a1",
      "short_name": "a1",
      "declarations": [],
      "spell": "3:32-3:34|0|3|2",
      "extent": "3:20-3:34|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 1
    }, {
      "id": 2,
      "usr": 1536316608590232194,
      "detailed_name": "Type a2",
      "short_name": "a2",
      "declarations": [],
      "spell": "4:8-4:10|0|3|2",
      "extent": "4:3-4:10|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 3,
      "usr": 316760354845869406,
      "detailed_name": "Type *a3",
      "short_name": "a3",
      "declarations": [],
      "spell": "5:9-5:11|0|3|2",
      "extent": "5:3-5:11|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 4,
      "usr": 12321730890779907974,
      "detailed_name": "const Type *a4",
      "short_name": "a4",
      "declarations": [],
      "spell": "6:15-6:17|0|3|2",
      "extent": "6:3-6:17|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 5,
      "usr": 4771437488905761633,
      "detailed_name": "const Type *const a5",
      "short_name": "a5",
      "hover": "const Type *const a5 = nullptr",
      "declarations": [],
      "spell": "7:21-7:23|0|3|2",
      "extent": "7:3-7:33|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
