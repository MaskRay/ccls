struct Foo {
  static Foo* Used();
};

void user() {
  Foo* x = Foo::Used();
}

/*

// TODO: Maybe only interesting usage of type is for function return type + variable declaration?
// TODO: Checking last location doesn't work for type usage all_uses...

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/foobar.cc:1:8",
      "all_uses": ["tests/foobar.cc:1:8", "tests/foobar.cc:2:10", "tests/foobar.cc:6:3", "tests/foobar.cc:6:12"],
      "interesting_uses": ["tests/foobar.cc:2:10", "tests/foobar.cc:6:3"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Used#S",
      "short_name": "Used",
      "qualified_name": "Foo::Used",
      "callers": ["1@tests/foobar.cc:6:17"],
      "all_uses": ["tests/foobar.cc:2:15", "tests/foobar.cc:6:17"]
    }, {
      "id": 1,
      "usr": "c:@F@user#",
      "short_name": "user",
      "qualified_name": "user",
      "definition": "tests/foobar.cc:5:6",
      "callees": ["0@tests/foobar.cc:6:17"],
      "all_uses": ["tests/foobar.cc:5:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:foobar.cc@60@F@user#@x",
      "short_name": "x",
      "qualified_name": "x",
      "declaration": "tests/foobar.cc:6:8",
      "variable_type": 0,
      "all_uses": ["tests/foobar.cc:6:8"]
    }]
}
*/