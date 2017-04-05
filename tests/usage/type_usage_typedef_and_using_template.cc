template<typename T>
struct Foo;

using Foo1 = Foo<int>;
typedef Foo<Foo1> Foo2;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo",
      "uses": ["2:8", "*4:14", "*5:9"]
    }, {
      "id": 1,
      "usr": "c:@Foo1",
      "short_name": "Foo1",
      "qualified_name": "Foo1",
      "definition": "4:7",
      "alias_of": 0,
      "uses": ["*4:7", "*5:13"]
    }, {
      "id": 2,
      "usr": "c:type_usage_typedef_and_using_template.cc@T@Foo2",
      "short_name": "Foo2",
      "qualified_name": "Foo2",
      "definition": "5:19",
      "alias_of": 0,
      "uses": ["*5:19"]
    }]
}
*/
