struct Type {
  Type() {}
};

void Make() {
  Type foo0;
  auto foo1 = Type();
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
      "declarations": ["2:3-2:7|-1|1|4"],
      "spell": "1:8-1:12|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [0, 1],
      "uses": ["2:3-2:7|0|2|4", "6:3-6:7|-1|1|4", "7:15-7:19|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 10530961286677896857,
      "detailed_name": "void Type::Type()",
      "short_name": "Type",
      "kind": 9,
      "storage": 1,
      "declarations": [],
      "spell": "2:3-2:7|0|2|2",
      "extent": "2:3-2:12|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:8-6:12|1|3|288", "7:15-7:19|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 3957104924306079513,
      "detailed_name": "void Make()",
      "short_name": "Make",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "5:6-5:10|-1|1|2",
      "extent": "5:1-8:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": ["6:8-6:12|0|3|288", "6:8-6:12|0|3|288", "7:15-7:19|0|3|32", "7:15-7:19|0|3|32"]
    }],
  "vars": [{
      "id": 0,
      "usr": 17348451315735351657,
      "detailed_name": "Type foo0",
      "short_name": "foo0",
      "declarations": [],
      "spell": "6:8-6:12|1|3|2",
      "extent": "6:3-6:12|1|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 3757978174345638825,
      "detailed_name": "Type foo1",
      "short_name": "foo1",
      "declarations": [],
      "spell": "7:8-7:12|1|3|2",
      "extent": "7:3-7:21|1|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
