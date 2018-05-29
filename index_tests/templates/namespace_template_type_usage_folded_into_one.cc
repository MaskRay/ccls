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
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 11072669167287398027,
      "detailed_name": "ns",
      "qual_name_offset": 0,
      "short_name": "ns",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:13|0|1|2",
      "extent": "1:1-7:2|0|1|0",
      "alias_of": 0,
      "bases": [13838176792705659279],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [{
          "L": 15768138241775955040,
          "R": -1
        }, {
          "L": 3182917058194750998,
          "R": -1
        }],
      "instances": [],
      "uses": ["1:11-1:13|0|1|4"]
    }, {
      "usr": 13838176792705659279,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [11072669167287398027],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 14042997404480181958,
      "detailed_name": "ns::Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "3:9-3:12|11072669167287398027|2|2",
      "extent": "3:3-3:15|11072669167287398027|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [15768138241775955040, 3182917058194750998],
      "uses": ["5:3-5:6|0|1|4", "6:3-6:6|0|1|4"]
    }],
  "usr2var": [{
      "usr": 3182917058194750998,
      "detailed_name": "Foo<bool> ns::b",
      "qual_name_offset": 10,
      "short_name": "b",
      "declarations": [],
      "spell": "6:13-6:14|11072669167287398027|2|2",
      "extent": "6:3-6:14|11072669167287398027|2|0",
      "type": 14042997404480181958,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 15768138241775955040,
      "detailed_name": "Foo<int> ns::a",
      "qual_name_offset": 9,
      "short_name": "a",
      "declarations": [],
      "spell": "5:12-5:13|11072669167287398027|2|2",
      "extent": "5:3-5:13|11072669167287398027|2|0",
      "type": 14042997404480181958,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
