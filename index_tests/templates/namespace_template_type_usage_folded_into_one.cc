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
  "usr2func": [],
  "usr2type": [{
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "bases": [],
      "funcs": [],
      "types": [14042997404480181958],
      "vars": [{
          "L": 15768138241775955040,
          "R": -1
        }, {
          "L": 3182917058194750998,
          "R": -1
        }],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["1:11-1:13|1:1-7:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 14042997404480181958,
      "detailed_name": "class ns::Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "3:9-3:12|3:3-3:15|1026|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 3,
      "declarations": [],
      "derived": [],
      "instances": [15768138241775955040, 3182917058194750998],
      "uses": ["5:3-5:6|4|-1", "6:3-6:6|4|-1"]
    }],
  "usr2var": [{
      "usr": 3182917058194750998,
      "detailed_name": "Foo<bool> ns::b",
      "qual_name_offset": 10,
      "short_name": "b",
      "spell": "6:13-6:14|6:3-6:14|1026|-1",
      "type": 14042997404480181958,
      "kind": 13,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 15768138241775955040,
      "detailed_name": "Foo<int> ns::a",
      "qual_name_offset": 9,
      "short_name": "a",
      "spell": "5:12-5:13|5:3-5:13|1026|-1",
      "type": 14042997404480181958,
      "kind": 13,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
