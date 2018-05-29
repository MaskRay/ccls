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
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 8221803074608342407,
      "detailed_name": "int ns::Foo::foo()",
      "qual_name_offset": 4,
      "short_name": "foo",
      "kind": 254,
      "storage": 3,
      "declarations": [],
      "spell": "5:16-5:19|14042997404480181958|2|2",
      "extent": "5:5-7:6|14042997404480181958|2|0",
      "declaring_type": 14042997404480181958,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["10:21-10:24|11072669167287398027|2|32", "11:22-11:25|11072669167287398027|2|32"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 17,
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
      "detailed_name": "ns",
      "qual_name_offset": 0,
      "short_name": "ns",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:13|0|1|2",
      "extent": "1:1-12:2|0|1|0",
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
      "spell": "3:10-3:13|11072669167287398027|2|2",
      "extent": "3:3-8:4|11072669167287398027|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [8221803074608342407],
      "vars": [],
      "instances": [],
      "uses": ["10:11-10:14|0|1|4", "11:11-11:14|0|1|4"]
    }],
  "usr2var": [{
      "usr": 3182917058194750998,
      "detailed_name": "int ns::b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int ns::b = Foo<bool>::foo<double>()",
      "declarations": [],
      "spell": "11:7-11:8|11072669167287398027|2|2",
      "extent": "11:3-11:35|11072669167287398027|2|0",
      "type": 17,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 15768138241775955040,
      "detailed_name": "int ns::a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int ns::a = Foo<int>::foo<float>()",
      "declarations": [],
      "spell": "10:7-10:8|11072669167287398027|2|2",
      "extent": "10:3-10:33|11072669167287398027|2|0",
      "type": 17,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
