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
  "types": [{
      "id": 0,
      "usr": "c:@N@ns@E@VarType",
      "short_name": "VarType",
      "detailed_name": "ns::VarType",
      "definition_spelling": "2:8-2:15",
      "definition_extent": "2:3-2:18",
      "instances": [0],
      "uses": ["2:8-2:15", "6:22-6:29", "6:44-6:51", "10:18-10:25"]
    }, {
      "id": 1,
      "usr": "c:@N@ns@ST>1#T@Holder",
      "short_name": "Holder",
      "detailed_name": "ns::Holder",
      "definition_spelling": "5:10-5:16",
      "definition_extent": "5:3-7:4",
      "vars": [0],
      "uses": ["5:10-5:16", "10:26-10:32", "13:13-13:19", "14:14-14:20"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@N@ns@ST>1#T@Holder@static_var",
      "short_name": "static_var",
      "detailed_name": "const ns::VarType ns::Holder::static_var",
      "declaration": "6:30-6:40",
      "definition_spelling": "10:37-10:47",
      "definition_extent": "9:3-10:47",
      "variable_type": 0,
      "declaring_type": 1,
      "uses": ["6:30-6:40", "10:37-10:47", "13:26-13:36", "14:27-14:37"]
    }, {
      "id": 1,
      "usr": "c:@N@ns@Foo",
      "short_name": "Foo",
      "detailed_name": "int ns::Foo",
      "definition_spelling": "13:7-13:10",
      "definition_extent": "13:3-13:36",
      "uses": ["13:7-13:10"]
    }, {
      "id": 2,
      "usr": "c:@N@ns@Foo2",
      "short_name": "Foo2",
      "detailed_name": "int ns::Foo2",
      "definition_spelling": "14:7-14:11",
      "definition_extent": "14:3-14:37",
      "uses": ["14:7-14:11"]
    }]
}
*/
