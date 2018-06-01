template<typename T>
void accept(T);

void foo() {
  accept(1);
  accept(true);
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "4:6-4:9|0|1|2",
      "extent": "4:1-7:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["5:3-5:9|10585861037135727329|3|32", "6:3-6:9|10585861037135727329|3|32"]
    }, {
      "usr": 10585861037135727329,
      "detailed_name": "void accept(T)",
      "qual_name_offset": 5,
      "short_name": "accept",
      "kind": 12,
      "storage": 0,
      "declarations": ["2:6-2:12|0|1|1"],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["5:3-5:9|4259594751088586730|3|32", "6:3-6:9|4259594751088586730|3|32"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 13420564603121289209,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "1:19-1:20|10585861037135727329|3|2",
      "extent": "1:10-1:20|10585861037135727329|3|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:13-2:14|0|1|4"]
    }],
  "usr2var": []
}
*/
