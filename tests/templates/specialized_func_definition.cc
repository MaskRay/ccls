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
      "definition": "1:2:7",
      "funcs": [0],
      "uses": ["*1:2:7", "*1:7:6", "1:9:6"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@ST>1#T@Template@F@Foo#",
      "short_name": "Foo",
      "qualified_name": "Template::Foo",
      "declarations": ["1:3:8", "1:9:22"],
      "definition": "1:7:19",
      "declaring_type": 0,
      "uses": ["1:3:8", "1:7:19", "1:9:22"]
    }],
  "variables": []
}
*/