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
      "qualified_name": "ns::VarType",
      "definition": "1:2:8",
      "instantiations": [0],
      "uses": ["*1:2:8", "*1:6:22", "*1:6:44", "*1:10:18"]
    }, {
      "id": 1,
      "usr": "c:@N@ns@ST>1#T@Holder",
      "short_name": "Holder",
      "qualified_name": "ns::Holder",
      "definition": "1:5:10",
      "vars": [0],
      "uses": ["*1:5:10", "*1:10:26", "1:13:13", "1:14:14"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@N@ns@ST>1#T@Holder@static_var",
      "short_name": "static_var",
      "qualified_name": "ns::Holder::static_var",
      "declaration": "1:6:30",
      "definition": "1:10:37",
      "variable_type": 0,
      "declaring_type": 1,
      "uses": ["1:6:30", "1:10:37", "1:13:26", "1:14:27"]
    }, {
      "id": 1,
      "usr": "c:@N@ns@Foo",
      "short_name": "Foo",
      "qualified_name": "ns::Foo",
      "definition": "1:13:7",
      "uses": ["1:13:7"]
    }, {
      "id": 2,
      "usr": "c:@N@ns@Foo2",
      "short_name": "Foo2",
      "qualified_name": "ns::Foo2",
      "definition": "1:14:7",
      "uses": ["1:14:7"]
    }]
}
*/
