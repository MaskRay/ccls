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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3957104924306079513,
      "detailed_name": "void Make()",
      "qual_name_offset": 5,
      "short_name": "Make",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "5:6-5:10|0|1|2",
      "extent": "5:1-8:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [449111627548814328, 17097499197730163115],
      "uses": [],
      "callees": ["6:8-6:12|10530961286677896857|3|16676", "6:8-6:12|10530961286677896857|3|16676", "7:15-7:19|10530961286677896857|3|16676", "7:15-7:19|10530961286677896857|3|16676"]
    }, {
      "usr": 10530961286677896857,
      "detailed_name": "Type::Type()",
      "qual_name_offset": 0,
      "short_name": "Type",
      "kind": 9,
      "storage": 0,
      "declarations": [],
      "spell": "2:3-2:7|13487927231218873822|2|1026",
      "extent": "2:3-2:12|13487927231218873822|2|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:8-6:12|3957104924306079513|3|16676", "7:15-7:19|3957104924306079513|3|16676"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 13487927231218873822,
      "detailed_name": "struct Type {}",
      "qual_name_offset": 7,
      "short_name": "Type",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:12|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [10530961286677896857],
      "vars": [],
      "instances": [449111627548814328, 17097499197730163115],
      "uses": ["2:3-2:7|13487927231218873822|2|4", "6:3-6:7|3957104924306079513|3|4", "7:15-7:19|3957104924306079513|3|4"]
    }],
  "usr2var": [{
      "usr": 449111627548814328,
      "detailed_name": "Type foo0",
      "qual_name_offset": 5,
      "short_name": "foo0",
      "declarations": [],
      "spell": "6:8-6:12|3957104924306079513|3|2",
      "extent": "6:3-6:12|3957104924306079513|3|0",
      "type": 13487927231218873822,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 17097499197730163115,
      "detailed_name": "Type foo1",
      "qual_name_offset": 5,
      "short_name": "foo1",
      "hover": "Type foo1 = Type()",
      "declarations": [],
      "spell": "7:8-7:12|3957104924306079513|3|2",
      "extent": "7:3-7:21|3957104924306079513|3|0",
      "type": 13487927231218873822,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
