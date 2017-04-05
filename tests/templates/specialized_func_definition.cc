template<class T>
class Template {
  void Foo();
};

template<class T>
void Template<T>::Foo() {}

void Template<void>::Foo() {}


/*
// TODO: usage information on Template is bad.
// TODO: Foo() should have multiple definitions.

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Template",
      "short_name": "Template",
      "qualified_name": "Template",
      "definition": "2:7",
      "funcs": [0],
      "uses": ["*2:7", "*7:6", "9:6"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@ST>1#T@Template@F@Foo#",
      "short_name": "Foo",
      "qualified_name": "Template::Foo",
      "declarations": ["3:8", "9:22"],
      "definition": "7:19",
      "declaring_type": 0,
      "uses": ["3:8", "7:19", "9:22"]
    }]
}
*/
