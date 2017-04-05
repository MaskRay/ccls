namespace ns {
  int Foo;
  void Accept(int a) {}
}

void Runner() {
  ns::Accept(ns::Foo);
  using namespace ns;
  Accept(Foo);
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@N@ns@F@Accept#I#",
      "short_name": "Accept",
      "qualified_name": "ns::Accept",
      "definition": "3:8",
      "callers": ["1@7:7", "1@9:3"],
      "uses": ["3:8", "7:7", "9:3"]
    }, {
      "id": 1,
      "usr": "c:@F@Runner#",
      "short_name": "Runner",
      "qualified_name": "Runner",
      "definition": "6:6",
      "callees": ["0@7:7", "0@9:3"],
      "uses": ["6:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@N@ns@Foo",
      "short_name": "Foo",
      "qualified_name": "ns::Foo",
      "definition": "2:7",
      "uses": ["2:7", "7:18", "9:10"]
    }, {
      "id": 1,
      "usr": "c:namespace_reference.cc@42@N@ns@F@Accept#I#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "3:19",
      "uses": ["3:19"]
    }]
}
*/



