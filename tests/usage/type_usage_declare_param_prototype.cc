struct Foo;

void foo(Foo* f, Foo*);
void foo(Foo* f, Foo*) {}

/*
// TODO: No interesting usage on prototype. But maybe that's ok!
// TODO: We should have the same variable declared for both prototype and
//       declaration. So it should have a usage marker on both. Then we could
//       rename parameters!

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "instances": [0],
      "uses": ["1:8-1:11", "3:10-3:13", "3:18-3:21", "4:10-4:13", "4:18-4:21"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@Foo#S0_#",
      "short_name": "foo",
      "detailed_name": "void foo(Foo *, Foo *)",
      "declarations": ["3:6-3:9"],
      "definition_spelling": "4:6-4:9",
      "definition_extent": "4:1-4:26"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_declare_param_prototype.cc@49@F@foo#*$@S@Foo#S0_#@f",
      "short_name": "f",
      "detailed_name": "Foo * f",
      "definition_spelling": "4:15-4:16",
      "definition_extent": "4:10-4:16",
      "variable_type": 0,
      "is_local": true,
      "uses": ["4:15-4:16"]
    }]
}
*/
