class Foo {
  Foo* make();
};

Foo* Foo::make() {
  Foo f;
  return nullptr;
}

extern Foo foo;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-3:2",
      "funcs": [0],
      "instances": [0, 1],
      "uses": ["1:7-1:10", "2:3-2:6", "5:1-5:4", "5:6-5:9", "6:3-6:6", "10:8-10:11"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Foo@F@make#",
      "short_name": "make",
      "detailed_name": "Foo *Foo::make()",
      "declarations": [{
          "spelling": "2:8-2:12",
          "extent": "2:3-2:14",
          "content": "Foo* make()"
        }],
      "definition_spelling": "5:11-5:15",
      "definition_extent": "5:1-8:2",
      "declaring_type": 0
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_various.cc@57@S@Foo@F@make#@f",
      "short_name": "f",
      "detailed_name": "Foo f",
      "definition_spelling": "6:7-6:8",
      "definition_extent": "6:3-6:8",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "uses": ["6:7-6:8"]
    }, {
      "id": 1,
      "usr": "c:@foo",
      "short_name": "foo",
      "detailed_name": "Foo foo",
      "declaration": "10:12-10:15",
      "variable_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["10:12-10:15"]
    }]
}
*/
