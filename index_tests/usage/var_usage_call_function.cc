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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:6-1:12|0|1|2",
      "extent": "1:1-1:17|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["4:13-4:19|0|1|132", "7:3-7:9|0|1|8228"],
      "callees": []
    }, {
      "usr": 11404881820527069090,
      "detailed_name": "void caller()",
      "qual_name_offset": 5,
      "short_name": "caller",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "3:6-3:12|0|1|2",
      "extent": "3:1-8:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [],
  "usr2var": [{
      "usr": 9121974011454213596,
      "detailed_name": "auto x",
      "qual_name_offset": 5,
      "short_name": "x",
      "hover": "auto x = &called",
      "declarations": [],
      "spell": "4:8-4:9|11404881820527069090|3|2",
      "extent": "4:3-4:19|11404881820527069090|3|0",
      "type": 0,
      "uses": ["5:3-5:4|11404881820527069090|3|8236"],
      "kind": 13,
      "storage": 0
    }]
}
*/
