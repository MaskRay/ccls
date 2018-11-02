void called() {}

void caller() {
  auto x = &called;
  x();

  called();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 468307235068920063,
      "detailed_name": "void called()",
      "qual_name_offset": 5,
      "short_name": "called",
      "spell": "1:6-1:12|1:1-1:17|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["4:13-4:19|132|-1", "7:3-7:9|16420|-1"]
    }, {
      "usr": 11404881820527069090,
      "detailed_name": "void caller()",
      "qual_name_offset": 5,
      "short_name": "caller",
      "spell": "3:6-3:12|3:1-8:2|2|-1",
      "bases": [],
      "vars": [9121974011454213596],
      "callees": ["4:13-4:19|468307235068920063|3|132", "4:13-4:19|468307235068920063|3|132", "7:3-7:9|468307235068920063|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [],
  "usr2var": [{
      "usr": 9121974011454213596,
      "detailed_name": "void (*)() x",
      "qual_name_offset": 11,
      "short_name": "x",
      "hover": "void (*)() x = &called",
      "spell": "4:8-4:9|4:3-4:19|2|-1",
      "type": 0,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["5:3-5:4|16428|-1"]
    }]
}
*/
