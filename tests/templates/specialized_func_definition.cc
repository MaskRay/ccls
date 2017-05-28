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
      "detailed_name": "Template",
      "definition_spelling": "2:7-2:15",
      "definition_extent": "2:1-4:2",
      "funcs": [0],
      "uses": ["2:7-2:15", "7:6-7:14", "9:6-9:14"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@ST>1#T@Template@F@Foo#",
      "short_name": "Foo",
      "detailed_name": "void Template::Foo()",
      "declarations": [{
          "spelling": "3:8-3:11",
          "extent": "3:3-3:13",
          "content": "void Foo()"
        }, {
          "spelling": "9:22-9:25",
          "extent": "9:1-9:30",
          "content": "void Template<void>::Foo() {}"
        }],
      "definition_spelling": "7:19-7:22",
      "definition_extent": "6:1-7:24",
      "declaring_type": 0
    }]
}
*/
