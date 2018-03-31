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
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 11072669167287398027,
      "detailed_name": "ns",
      "short_name": "ns",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:13|-1|1|2",
      "extent": "1:1-15:2|-1|1|0",
      "bases": [1],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [1, 2],
      "instances": [],
      "uses": ["1:11-1:13|-1|1|4"]
    }, {
      "id": 1,
      "usr": 13838176792705659279,
      "detailed_name": "<fundamental>",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [0],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 2,
      "usr": 1532099849728741556,
      "detailed_name": "ns::VarType",
      "short_name": "VarType",
      "kind": 10,
      "declarations": [],
      "spell": "2:8-2:15|0|2|2",
      "extent": "2:3-2:18|0|2|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["6:22-6:29|-1|1|4", "6:44-6:51|-1|1|4", "10:18-10:25|-1|1|4"]
    }, {
      "id": 3,
      "usr": 12688716854043726585,
      "detailed_name": "ns::Holder",
      "short_name": "Holder",
      "kind": 5,
      "declarations": [],
      "spell": "5:10-5:16|0|2|2",
      "extent": "5:3-7:4|0|2|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0],
      "instances": [],
      "uses": ["10:26-10:32|-1|1|4", "13:13-13:19|-1|1|4", "14:14-14:20|-1|1|4"]
    }, {
      "id": 4,
      "usr": 14511917000226829276,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["10:33-10:34|-1|1|4"]
    }, {
      "id": 5,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [1, 2],
      "uses": []
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 4731849186641714451,
      "detailed_name": "const ns::VarType ns::Holder::static_var",
      "short_name": "static_var",
      "hover": "const ns::VarType ns::Holder::static_var = (VarType)0x0",
      "declarations": ["6:30-6:40|3|2|1"],
      "spell": "10:37-10:47|3|2|2",
      "extent": "9:3-10:47|0|2|0",
      "type": 2,
      "uses": ["13:26-13:36|-1|1|4", "14:27-14:37|-1|1|4"],
      "kind": 8,
      "storage": 1
    }, {
      "id": 1,
      "usr": 12898699035586282159,
      "detailed_name": "int ns::Foo",
      "short_name": "Foo",
      "hover": "int ns::Foo = Holder<int>::static_var",
      "declarations": [],
      "spell": "13:7-13:10|0|2|2",
      "extent": "13:3-13:36|0|2|0",
      "type": 5,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 2,
      "usr": 9008550860229740818,
      "detailed_name": "int ns::Foo2",
      "short_name": "Foo2",
      "hover": "int ns::Foo2 = Holder<int>::static_var",
      "declarations": [],
      "spell": "14:7-14:11|0|2|2",
      "extent": "14:3-14:37|0|2|0",
      "type": 5,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
