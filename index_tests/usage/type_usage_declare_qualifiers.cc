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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 16858540520096802573,
      "detailed_name": "void foo(Type &a0, const Type &a1)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "3:6-3:9|3:1-8:2|2|-1",
      "bases": [],
      "vars": [7997456978847868736, 17228576662112939520, 15429032129697337561, 6081981442495435784, 5004072032239834773, 14939253431683105646],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 13487927231218873822,
      "detailed_name": "struct Type {}",
      "qual_name_offset": 7,
      "short_name": "Type",
      "spell": "1:8-1:12|1:1-1:15|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [7997456978847868736, 17228576662112939520, 15429032129697337561, 6081981442495435784, 5004072032239834773, 14939253431683105646],
      "uses": ["3:10-3:14|4|-1", "3:26-3:30|4|-1", "4:3-4:7|4|-1", "5:3-5:7|4|-1", "6:9-6:13|4|-1", "7:9-7:13|4|-1"]
    }],
  "usr2var": [{
      "usr": 5004072032239834773,
      "detailed_name": "const Type *a4",
      "qual_name_offset": 12,
      "short_name": "a4",
      "spell": "6:15-6:17|6:3-6:17|2|-1",
      "type": 13487927231218873822,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 6081981442495435784,
      "detailed_name": "Type *a3",
      "qual_name_offset": 6,
      "short_name": "a3",
      "spell": "5:9-5:11|5:3-5:11|2|-1",
      "type": 13487927231218873822,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 7997456978847868736,
      "detailed_name": "Type &a0",
      "qual_name_offset": 6,
      "short_name": "a0",
      "spell": "3:16-3:18|3:10-3:18|1026|-1",
      "type": 13487927231218873822,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 14939253431683105646,
      "detailed_name": "const Type *const a5",
      "qual_name_offset": 18,
      "short_name": "a5",
      "hover": "const Type *const a5 = nullptr",
      "spell": "7:21-7:23|7:3-7:33|2|-1",
      "type": 13487927231218873822,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 15429032129697337561,
      "detailed_name": "Type a2",
      "qual_name_offset": 5,
      "short_name": "a2",
      "spell": "4:8-4:10|4:3-4:10|2|-1",
      "type": 13487927231218873822,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 17228576662112939520,
      "detailed_name": "const Type &a1",
      "qual_name_offset": 12,
      "short_name": "a1",
      "spell": "3:32-3:34|3:20-3:34|1026|-1",
      "type": 13487927231218873822,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
