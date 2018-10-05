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
      "spell": "5:16-5:19|5:5-7:6|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 254,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["10:21-10:24|36|-1", "11:22-11:25|36|-1"]
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [15768138241775955040, 3182917058194750998],
      "uses": []
    }, {
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
      "declarations": ["1:11-1:13|1:1-12:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 14042997404480181958,
      "detailed_name": "struct ns::Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "3:10-3:13|3:3-8:4|1026|-1",
      "bases": [],
      "funcs": [8221803074608342407],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 3,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["10:11-10:14|4|-1", "11:11-11:14|4|-1"]
    }],
  "usr2var": [{
      "usr": 3182917058194750998,
      "detailed_name": "int ns::b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int ns::b = Foo<bool>::foo<double>()",
      "spell": "11:7-11:8|11:3-11:35|1026|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 15768138241775955040,
      "detailed_name": "int ns::a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int ns::a = Foo<int>::foo<float>()",
      "spell": "10:7-10:8|10:3-10:33|1026|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
