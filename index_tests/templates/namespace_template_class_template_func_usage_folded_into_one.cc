namespace ns {
  template<typename T>
  struct Foo {
    template<typename R>
    static int foo() {
      return 3;
    }
  };

  int a = Foo<int>::foo<float>();
  int b = Foo<bool>::foo<double>();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 8221803074608342407,
      "detailed_name": "static int ns::Foo::foo()",
      "qual_name_offset": 11,
      "short_name": "foo",
      "kind": 254,
      "storage": 0,
      "declarations": [],
      "spell": "5:16-5:19|14042997404480181958|2|1026|-1",
      "extent": "5:5-7:6|14042997404480181958|2|0|-1",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["10:21-10:24|11072669167287398027|2|36|-1", "11:22-11:25|11072669167287398027|2|36|-1"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [15768138241775955040, 3182917058194750998],
      "uses": []
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "kind": 3,
      "declarations": ["1:11-1:13|1:1-12:2|0|1|1|-1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [14042997404480181958],
      "funcs": [],
      "vars": [{
          "L": 15768138241775955040,
          "R": -1
        }, {
          "L": 3182917058194750998,
          "R": -1
        }],
      "instances": [],
      "uses": []
    }, {
      "usr": 14042997404480181958,
      "detailed_name": "struct ns::Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "3:10-3:13|11072669167287398027|2|1026|-1",
      "extent": "3:3-8:4|11072669167287398027|2|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [8221803074608342407],
      "vars": [],
      "instances": [],
      "uses": ["10:11-10:14|11072669167287398027|2|4|-1", "11:11-11:14|11072669167287398027|2|4|-1"]
    }],
  "usr2var": [{
      "usr": 3182917058194750998,
      "detailed_name": "int ns::b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int ns::b = Foo<bool>::foo<double>()",
      "declarations": [],
      "spell": "11:7-11:8|11072669167287398027|2|1026|-1",
      "extent": "11:3-11:35|11072669167287398027|2|0|-1",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 15768138241775955040,
      "detailed_name": "int ns::a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int ns::a = Foo<int>::foo<float>()",
      "declarations": [],
      "spell": "10:7-10:8|11072669167287398027|2|1026|-1",
      "extent": "10:3-10:33|11072669167287398027|2|0|-1",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
