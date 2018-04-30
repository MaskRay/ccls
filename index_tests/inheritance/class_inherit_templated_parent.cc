template<unsigned int C>
class Base1 {};

template<typename C>
class Base2 {};

template<unsigned int T>
class Derived1 : Base1<T> {};

template<typename T>
class Derived2 : Base2<T> {};

class Derived : Base1<3>, Base2<Derived>, Derived1<4>, Derived2<Derived> {};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 9,
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
      "instances": [12990052348105569112],
      "uses": []
    }, {
      "usr": 5863733211528032190,
      "detailed_name": "Derived1",
      "qual_name_offset": 0,
      "short_name": "Derived1",
      "kind": 5,
      "declarations": ["13:43-13:51|0|1|4"],
      "spell": "8:7-8:15|0|1|2",
      "extent": "8:1-8:29|0|1|0",
      "alias_of": 0,
      "bases": [11930058224338108382],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:43-13:51|0|1|4"]
    }, {
      "usr": 7916588271848318236,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "10:19-10:20|0|1|2",
      "extent": "10:10-10:20|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["11:24-11:25|0|1|4"]
    }, {
      "usr": 10651399730831737929,
      "detailed_name": "Derived2",
      "qual_name_offset": 0,
      "short_name": "Derived2",
      "kind": 5,
      "declarations": ["13:56-13:64|0|1|4"],
      "spell": "11:7-11:15|0|1|2",
      "extent": "11:1-11:29|0|1|0",
      "alias_of": 0,
      "bases": [11118288764693061434],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:56-13:64|0|1|4"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "Derived",
      "qual_name_offset": 0,
      "short_name": "Derived",
      "kind": 5,
      "declarations": ["13:33-13:40|0|1|4", "13:65-13:72|0|1|4"],
      "spell": "13:7-13:14|0|1|2",
      "extent": "13:1-13:76|0|1|0",
      "alias_of": 0,
      "bases": [11930058224338108382, 11118288764693061434, 5863733211528032190, 10651399730831737929],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:33-13:40|0|1|4", "13:65-13:72|0|1|4"]
    }, {
      "usr": 11118288764693061434,
      "detailed_name": "Base2",
      "qual_name_offset": 0,
      "short_name": "Base2",
      "kind": 5,
      "declarations": ["11:18-11:23|0|1|4", "13:27-13:32|0|1|4"],
      "spell": "5:7-5:12|0|1|2",
      "extent": "5:1-5:15|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [10651399730831737929, 10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["11:18-11:23|0|1|4", "13:27-13:32|0|1|4"]
    }, {
      "usr": 11930058224338108382,
      "detailed_name": "Base1",
      "qual_name_offset": 0,
      "short_name": "Base1",
      "kind": 5,
      "declarations": ["8:18-8:23|0|1|4", "13:17-13:22|0|1|4"],
      "spell": "2:7-2:12|0|1|2",
      "extent": "2:1-2:15|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [5863733211528032190, 10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["8:18-8:23|0|1|4", "13:17-13:22|0|1|4"]
    }],
  "usr2var": [{
      "usr": 12990052348105569112,
      "detailed_name": "unsigned int T",
      "qual_name_offset": 13,
      "short_name": "",
      "declarations": [],
      "spell": "7:23-7:24|0|1|2",
      "extent": "7:10-7:24|0|1|0",
      "type": 9,
      "uses": ["8:24-8:25|0|1|4"],
      "kind": 26,
      "storage": 0
    }]
}
*/
