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
  "usr2func": [{
      "usr": 16858540520096802573,
      "detailed_name": "void foo(Type &a0, const Type &a1)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "3:6-3:9|0|1|2",
      "extent": "3:1-8:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [7997456978847868736, 17228576662112939520, 15429032129697337561, 6081981442495435784, 5004072032239834773, 14939253431683105646],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 13487927231218873822,
      "detailed_name": "Type",
      "qual_name_offset": 0,
      "short_name": "Type",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:12|0|1|2",
      "extent": "1:1-1:15|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [7997456978847868736, 17228576662112939520, 15429032129697337561, 6081981442495435784, 5004072032239834773, 14939253431683105646],
      "uses": ["3:10-3:14|0|1|4", "3:26-3:30|0|1|4", "4:3-4:7|0|1|4", "5:3-5:7|0|1|4", "6:9-6:13|0|1|4", "7:9-7:13|0|1|4"]
    }],
  "usr2var": [{
      "usr": 5004072032239834773,
      "detailed_name": "const Type *a4",
      "qual_name_offset": 12,
      "short_name": "a4",
      "declarations": [],
      "spell": "6:15-6:17|16858540520096802573|3|2",
      "extent": "6:3-6:17|16858540520096802573|3|0",
      "type": 13487927231218873822,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 6081981442495435784,
      "detailed_name": "Type *a3",
      "qual_name_offset": 6,
      "short_name": "a3",
      "declarations": [],
      "spell": "5:9-5:11|16858540520096802573|3|2",
      "extent": "5:3-5:11|16858540520096802573|3|0",
      "type": 13487927231218873822,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 7997456978847868736,
      "detailed_name": "Type &a0",
      "qual_name_offset": 6,
      "short_name": "a0",
      "declarations": [],
      "spell": "3:16-3:18|16858540520096802573|3|2",
      "extent": "3:10-3:18|16858540520096802573|3|0",
      "type": 13487927231218873822,
      "uses": [],
      "kind": 253,
      "storage": 0
    }, {
      "usr": 14939253431683105646,
      "detailed_name": "const Type *const a5",
      "qual_name_offset": 18,
      "short_name": "a5",
      "hover": "const Type *const a5 = nullptr",
      "declarations": [],
      "spell": "7:21-7:23|16858540520096802573|3|2",
      "extent": "7:3-7:33|16858540520096802573|3|0",
      "type": 13487927231218873822,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 15429032129697337561,
      "detailed_name": "Type a2",
      "qual_name_offset": 5,
      "short_name": "a2",
      "declarations": [],
      "spell": "4:8-4:10|16858540520096802573|3|2",
      "extent": "4:3-4:10|16858540520096802573|3|0",
      "type": 13487927231218873822,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 17228576662112939520,
      "detailed_name": "const Type &a1",
      "qual_name_offset": 12,
      "short_name": "a1",
      "declarations": [],
      "spell": "3:32-3:34|16858540520096802573|3|2",
      "extent": "3:20-3:34|16858540520096802573|3|0",
      "type": 13487927231218873822,
      "uses": [],
      "kind": 253,
      "storage": 0
    }]
}
*/
