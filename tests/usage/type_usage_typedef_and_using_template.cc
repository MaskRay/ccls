template<typename T>
struct Foo;

using Foo1 = Foo<int>;
typedef Foo<Foo1> Foo2;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "",
      "detailed_name": "",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:8-2:11", "4:14-4:17", "5:9-5:12"]
    }, {
      "id": 1,
      "usr": "c:@Foo1",
      "short_name": "Foo1",
      "detailed_name": "using Foo1 = Foo<int>",
      "definition_spelling": "4:7-4:11",
      "definition_extent": "4:1-4:22",
      "alias_of": 0,
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:7-4:11", "5:13-5:17"]
    }, {
      "id": 2,
      "usr": "c:type_usage_typedef_and_using_template.cc@T@Foo2",
      "short_name": "Foo2",
      "detailed_name": "typedef Foo<Foo1> Foo2",
      "definition_spelling": "5:19-5:23",
      "definition_extent": "5:1-5:23",
      "alias_of": 0,
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["5:19-5:23"]
    }],
  "funcs": [],
  "vars": []
}
*/
