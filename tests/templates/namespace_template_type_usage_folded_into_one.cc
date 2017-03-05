namespace ns {
  template<typename T>
  class Foo {};

  Foo<int> a;
  Foo<bool> b;
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@N@ns@ST>1#T@Foo",
      "short_name": "Foo",
      "qualified_name": "ns::Foo",
      "definition": "1:3:9",
      "uses": ["*1:3:9", "*1:5:3", "*1:6:3"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@N@ns@a",
      "short_name": "a",
      "qualified_name": "ns::a",
      "definition": "1:5:12",
      "variable_type": 0,
      "uses": ["1:5:12"]
    }, {
      "id": 1,
      "usr": "c:@N@ns@b",
      "short_name": "b",
      "qualified_name": "ns::b",
      "definition": "1:6:13",
      "variable_type": 0,
      "uses": ["1:6:13"]
    }]
}
*/