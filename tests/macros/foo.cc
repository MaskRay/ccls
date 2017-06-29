#define A 5
#define DISALLOW(type) type(type&&) = delete;

struct Foo {
  DISALLOW(Foo);
};

int x = A;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "4:8-4:11",
      "definition_extent": "4:1-6:2",
      "uses": ["4:8-4:11"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@x",
      "short_name": "x",
      "detailed_name": "int x",
      "definition_spelling": "8:5-8:6",
      "definition_extent": "8:1-8:10",
      "is_local": false,
      "is_macro": false,
      "uses": ["8:5-8:6"]
    }, {
      "id": 1,
      "usr": "c:foo.cc@8@macro@A",
      "short_name": "A",
      "detailed_name": "A",
      "definition_spelling": "1:9-1:10",
      "definition_extent": "1:9-1:12",
      "is_local": false,
      "is_macro": true,
      "uses": ["1:9-1:10", "8:9-8:10"]
    }, {
      "id": 2,
      "usr": "c:foo.cc@21@macro@DISALLOW",
      "short_name": "DISALLOW",
      "detailed_name": "DISALLOW",
      "definition_spelling": "2:9-2:17",
      "definition_extent": "2:9-2:46",
      "is_local": false,
      "is_macro": true,
      "uses": ["2:9-2:17", "5:3-5:11"]
    }]
}
*/
