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
      "qualified_name": "Foo",
      "definition": "1:7",
      "funcs": [0],
      "instantiations": [0, 1],
      "uses": ["*1:7", "*2:3", "*5:1", "5:6", "*6:3", "*10:8"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@make#",
      "short_name": "make",
      "qualified_name": "Foo::make",
      "declarations": ["2:8"],
      "definition": "5:11",
      "declaring_type": 0,
      "uses": ["2:8", "5:11"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_various.cc@57@S@Foo@F@make#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "6:7",
      "variable_type": 0,
      "uses": ["6:7"]
    }, {
      "id": 1,
      "usr": "c:@foo",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "10:12",
      "variable_type": 0,
      "uses": ["10:12"]
    }]
}
*/
