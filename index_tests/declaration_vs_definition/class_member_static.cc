class Foo {
  static int foo;
};

int Foo::foo;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 17,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [8942920329766232482],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [8942920329766232482],
      "instances": [],
      "uses": ["5:5-5:8|0|1|4"]
    }],
  "usr2var": [{
      "usr": 8942920329766232482,
      "detailed_name": "int Foo::foo",
      "qual_name_offset": 4,
      "short_name": "foo",
      "declarations": ["2:14-2:17|15041163540773201510|2|1"],
      "spell": "5:10-5:13|15041163540773201510|2|2",
      "extent": "5:1-5:13|0|1|0",
      "type": 17,
      "uses": [],
      "kind": 8,
      "storage": 1
    }]
}
*/
