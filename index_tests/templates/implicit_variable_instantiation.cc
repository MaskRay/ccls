namespace ns {
  enum VarType {};

  template<typename _>
  struct Holder {
    static constexpr VarType static_var = (VarType)0x0;
  };

  template<typename _>
  const typename VarType Holder<_>::static_var;


  int Foo = Holder<int>::static_var;
  int Foo2 = Holder<int>::static_var;
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
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
      "instances": [12898699035586282159, 9008550860229740818],
      "uses": []
    }, {
      "usr": 1532099849728741556,
      "detailed_name": "enum ns::VarType {\n}",
      "qual_name_offset": 5,
      "short_name": "VarType",
      "kind": 10,
      "declarations": [],
      "spell": "2:8-2:15|11072669167287398027|2|1026",
      "extent": "2:3-2:18|11072669167287398027|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [4731849186641714451, 4731849186641714451],
      "uses": ["6:22-6:29|11072669167287398027|2|4", "6:44-6:51|11072669167287398027|2|4", "10:18-10:25|11072669167287398027|2|4"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {\n}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "kind": 3,
      "declarations": ["1:11-1:13|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [1532099849728741556, 12688716854043726585],
      "funcs": [],
      "vars": [{
          "L": 12898699035586282159,
          "R": -1
        }, {
          "L": 9008550860229740818,
          "R": -1
        }],
      "instances": [],
      "uses": []
    }, {
      "usr": 12688716854043726585,
      "detailed_name": "struct ns::Holder {}",
      "qual_name_offset": 7,
      "short_name": "Holder",
      "kind": 23,
      "declarations": [],
      "spell": "5:10-5:16|11072669167287398027|2|1026",
      "extent": "5:3-7:4|11072669167287398027|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["10:26-10:32|11072669167287398027|2|4", "13:13-13:19|11072669167287398027|2|4", "14:14-14:20|11072669167287398027|2|4"]
    }],
  "usr2var": [{
      "usr": 4731849186641714451,
      "detailed_name": "static constexpr ns::VarType ns::Holder::static_var",
      "qual_name_offset": 29,
      "short_name": "static_var",
      "hover": "static constexpr ns::VarType ns::Holder::static_var = (VarType)0x0",
      "declarations": ["6:30-6:40|12688716854043726585|2|1025"],
      "spell": "10:37-10:47|12688716854043726585|2|1026",
      "extent": "9:3-10:47|11072669167287398027|2|0",
      "type": 1532099849728741556,
      "uses": ["13:26-13:36|11072669167287398027|2|12", "14:27-14:37|11072669167287398027|2|12"],
      "kind": 13,
      "storage": 2
    }, {
      "usr": 9008550860229740818,
      "detailed_name": "int ns::Foo2",
      "qual_name_offset": 4,
      "short_name": "Foo2",
      "hover": "int ns::Foo2 = Holder<int>::static_var",
      "declarations": [],
      "spell": "14:7-14:11|11072669167287398027|2|1026",
      "extent": "14:3-14:37|11072669167287398027|2|0",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 12898699035586282159,
      "detailed_name": "int ns::Foo",
      "qual_name_offset": 4,
      "short_name": "Foo",
      "hover": "int ns::Foo = Holder<int>::static_var",
      "declarations": [],
      "spell": "13:7-13:10|11072669167287398027|2|1026",
      "extent": "13:3-13:36|11072669167287398027|2|0",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
