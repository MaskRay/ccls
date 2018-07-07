namespace ns {
  template<typename T>
  class Foo {};

  Foo<int> a;
  Foo<bool> b;
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 11072669167287398027,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [15768138241775955040, 3182917058194750998],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {\n}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "kind": 3,
      "declarations": ["1:11-1:13|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [14042997404480181958],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 14042997404480181958,
      "detailed_name": "class ns::Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "3:9-3:12|11072669167287398027|2|514",
      "extent": "3:3-3:15|11072669167287398027|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [15768138241775955040, 3182917058194750998],
      "uses": ["5:3-5:6|11072669167287398027|2|4", "6:3-6:6|11072669167287398027|2|4"]
    }],
  "usr2var": [{
      "usr": 3182917058194750998,
      "detailed_name": "Foo<ns::bool> b",
      "qual_name_offset": 4,
      "short_name": "b",
      "declarations": [],
      "spell": "6:13-6:14|11072669167287398027|2|514",
      "extent": "6:3-6:14|11072669167287398027|2|0",
      "type": 14042997404480181958,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 15768138241775955040,
      "detailed_name": "Foo<int> ns::a",
      "qual_name_offset": 9,
      "short_name": "a",
      "declarations": [],
      "spell": "5:12-5:13|11072669167287398027|2|514",
      "extent": "5:3-5:13|11072669167287398027|2|0",
      "type": 14042997404480181958,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
