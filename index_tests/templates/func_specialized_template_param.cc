template<class T>
class Template {};

struct Foo {
  void Bar(Template<double>&);
};

void Foo::Bar(Template<double>&) {}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 17107291254533526269,
      "detailed_name": "Template",
      "short_name": "Template",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:15|-1|1|2",
      "extent": "2:1-2:18|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["5:12-5:20|-1|1|4", "8:15-8:23|-1|1|4"]
    }, {
      "id": 1,
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "4:8-4:11|-1|1|2",
      "extent": "4:1-6:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["8:6-8:9|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 8412238651648388423,
      "detailed_name": "void Foo::Bar(Template<double> &)",
      "short_name": "Bar",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "5:8-5:11|1|2|1",
          "param_spellings": ["5:29-5:29"]
        }],
      "spell": "8:11-8:14|1|2|2",
      "extent": "8:1-8:36|-1|1|0",
      "declaring_type": 1,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/
