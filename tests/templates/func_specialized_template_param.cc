template<class T>
class Template {};

struct Foo {
  void Bar(Template<double>&);
};

void Foo::Bar(Template<double>&) {}

/*
OUTPUT:
{
  "last_modification_time": 1,
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Template",
      "short_name": "Template",
      "detailed_name": "Template",
      "definition_spelling": "2:7-2:15",
      "definition_extent": "2:1-2:18",
      "uses": ["2:7-2:15", "5:12-5:20", "8:15-8:23"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "4:8-4:11",
      "definition_extent": "4:1-6:2",
      "funcs": [0],
      "uses": ["4:8-4:11", "8:6-8:9"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Bar#&$@S@Template>#d#",
      "short_name": "Bar",
      "detailed_name": "void Foo::Bar(Template<double> &)",
      "declarations": ["5:8-5:11"],
      "definition_spelling": "8:11-8:14",
      "definition_extent": "8:1-8:36",
      "declaring_type": 1
    }]
}
*/
