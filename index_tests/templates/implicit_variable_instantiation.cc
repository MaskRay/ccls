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
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [12898699035586282159, 9008550860229740818],
      "uses": []
    }, {
      "usr": 1532099849728741556,
      "detailed_name": "enum ns::VarType {}",
      "qual_name_offset": 5,
      "short_name": "VarType",
      "spell": "2:8-2:15|2:3-2:18|1026|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 3,
      "declarations": [],
      "derived": [],
      "instances": [4731849186641714451, 4731849186641714451],
      "uses": ["6:22-6:29|4|-1", "6:44-6:51|4|-1", "10:18-10:25|4|-1"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "bases": [],
      "funcs": [],
      "types": [1532099849728741556, 12688716854043726585],
      "vars": [{
          "L": 12898699035586282159,
          "R": -1
        }, {
          "L": 9008550860229740818,
          "R": -1
        }],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["1:11-1:13|1:1-15:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 12688716854043726585,
      "detailed_name": "struct ns::Holder {}",
      "qual_name_offset": 7,
      "short_name": "Holder",
      "spell": "5:10-5:16|5:3-7:4|1026|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 3,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["10:26-10:32|4|-1", "13:13-13:19|4|-1", "14:14-14:20|4|-1"]
    }],
  "usr2var": [{
      "usr": 4731849186641714451,
      "detailed_name": "static constexpr ns::VarType ns::Holder::static_var",
      "qual_name_offset": 29,
      "short_name": "static_var",
      "hover": "static constexpr ns::VarType ns::Holder::static_var = (VarType)0x0",
      "spell": "10:37-10:47|9:3-10:47|1026|-1",
      "type": 1532099849728741556,
      "kind": 13,
      "parent_kind": 23,
      "storage": 2,
      "declarations": ["6:30-6:40|6:5-6:55|1025|-1"],
      "uses": ["13:26-13:36|12|-1", "14:27-14:37|12|-1"]
    }, {
      "usr": 9008550860229740818,
      "detailed_name": "int ns::Foo2",
      "qual_name_offset": 4,
      "short_name": "Foo2",
      "hover": "int ns::Foo2 = Holder<int>::static_var",
      "spell": "14:7-14:11|14:3-14:37|1026|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 12898699035586282159,
      "detailed_name": "int ns::Foo",
      "qual_name_offset": 4,
      "short_name": "Foo",
      "hover": "int ns::Foo = Holder<int>::static_var",
      "spell": "13:7-13:10|13:3-13:36|1026|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
