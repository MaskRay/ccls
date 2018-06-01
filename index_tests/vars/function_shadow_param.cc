void foo(int p) {
  { int p = 0; }
}
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 11998306017310352355,
      "detailed_name": "void foo(int p)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:6-1:9|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [5875271969926422921, 11404600766177939811],
      "uses": [],
      "callees": []
    }],
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
      "instances": [5875271969926422921, 11404600766177939811],
      "uses": []
    }],
  "usr2var": [{
      "usr": 5875271969926422921,
      "detailed_name": "int p",
      "qual_name_offset": 4,
      "short_name": "p",
      "declarations": [],
      "spell": "1:14-1:15|11998306017310352355|3|2",
      "extent": "1:10-1:15|11998306017310352355|3|0",
      "type": 17,
      "uses": [],
      "kind": 253,
      "storage": 0
    }, {
      "usr": 11404600766177939811,
      "detailed_name": "int p",
      "qual_name_offset": 4,
      "short_name": "p",
      "hover": "int p = 0",
      "declarations": [],
      "spell": "2:9-2:10|11998306017310352355|3|2",
      "extent": "2:5-2:14|11998306017310352355|3|0",
      "type": 17,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
