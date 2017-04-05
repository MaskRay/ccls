template<class T>
class Template {};

struct Foo {
  void Bar(Template<double>&);
};

void Foo::Bar(Template<double>&) {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Template",
      "short_name": "Template",
      "qualified_name": "Template",
      "definition": "2:7",
      "uses": ["*2:7", "5:12", "*8:15"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "4:8",
      "funcs": [0],
      "uses": ["*4:8", "8:6"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Bar#&$@S@Template>#d#",
      "short_name": "Bar",
      "qualified_name": "Foo::Bar",
      "declarations": ["5:8"],
      "definition": "8:11",
      "declaring_type": 1,
      "uses": ["5:8", "8:11"]
    }]
}
*/
