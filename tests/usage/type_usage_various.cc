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
      "definition": "1:1:7",
      "funcs": [0],
      "uses": ["1:1:7", "*1:2:3", "*1:5:1", "1:5:6", "*1:6:3", "*1:10:8"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@make#",
      "short_name": "make",
      "qualified_name": "Foo::make",
      "declaration": "1:2:8",
      "definition": "1:5:11",
      "declaring_type": 0,
      "uses": ["1:2:8", "1:5:11"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_various.cc@58@S@Foo@F@make#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "1:6:7",
      "variable_type": 0,
      "uses": ["1:6:7"]
    }, {
      "id": 1,
      "usr": "c:@foo",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "1:10:12",
      "variable_type": 0,
      "uses": ["1:10:12"]
    }]
}
*/