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
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 444,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [2713508113307684499, 12990052348105569112],
      "uses": []
    }, {
      "usr": 5863733211528032190,
      "detailed_name": "class Derived1 : Base1<T> {}",
      "qual_name_offset": 6,
      "short_name": "Derived1",
      "spell": "8:7-8:15|8:1-8:29|2|-1",
      "bases": [11930058224338108382],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 1,
      "declarations": [],
      "derived": [10963370434658308541],
      "instances": [],
      "uses": ["13:43-13:51|2052|-1"]
    }, {
      "usr": 7916588271848318236,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 0,
      "declarations": ["10:19-10:20|10:10-10:20|1025|-1"],
      "derived": [],
      "instances": [],
      "uses": ["11:24-11:25|4|-1"]
    }, {
      "usr": 10651399730831737929,
      "detailed_name": "class Derived2 : Base2<T> {}",
      "qual_name_offset": 6,
      "short_name": "Derived2",
      "spell": "11:7-11:15|11:1-11:29|2|-1",
      "bases": [11118288764693061434],
      "funcs": [],
      "types": [7916588271848318236],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 1,
      "declarations": [],
      "derived": [10963370434658308541],
      "instances": [],
      "uses": ["13:56-13:64|2052|-1"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "class Derived : Base1<3>, Base2<Derived>, Derived1<4>, Derived2<Derived> {}",
      "qual_name_offset": 6,
      "short_name": "Derived",
      "spell": "13:7-13:14|13:1-13:76|2|-1",
      "bases": [11930058224338108382, 11118288764693061434, 5863733211528032190, 10651399730831737929],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["13:33-13:40|4|-1", "13:65-13:72|4|-1"]
    }, {
      "usr": 11118288764693061434,
      "detailed_name": "class Base2 {}",
      "qual_name_offset": 6,
      "short_name": "Base2",
      "spell": "5:7-5:12|5:1-5:15|2|-1",
      "bases": [],
      "funcs": [],
      "types": [11972372958367790157],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 1,
      "declarations": [],
      "derived": [10651399730831737929, 10963370434658308541],
      "instances": [],
      "uses": ["11:18-11:23|2052|-1", "13:27-13:32|2052|-1"]
    }, {
      "usr": 11930058224338108382,
      "detailed_name": "class Base1 {}",
      "qual_name_offset": 6,
      "short_name": "Base1",
      "spell": "2:7-2:12|2:1-2:15|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 1,
      "declarations": [],
      "derived": [5863733211528032190, 10963370434658308541],
      "instances": [],
      "uses": ["8:18-8:23|2052|-1", "13:17-13:22|2052|-1"]
    }, {
      "usr": 11972372958367790157,
      "detailed_name": "C",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 0,
      "declarations": ["4:19-4:20|4:10-4:20|1025|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 2713508113307684499,
      "detailed_name": "unsigned int Base1::C",
      "qual_name_offset": 13,
      "short_name": "C",
      "type": 444,
      "kind": 253,
      "parent_kind": 5,
      "storage": 0,
      "declarations": ["1:23-1:24|1:10-1:24|1025|-1"],
      "uses": []
    }, {
      "usr": 12990052348105569112,
      "detailed_name": "unsigned int Derived1::T",
      "qual_name_offset": 13,
      "short_name": "T",
      "type": 444,
      "kind": 253,
      "parent_kind": 5,
      "storage": 0,
      "declarations": ["7:23-7:24|7:10-7:24|1025|-1"],
      "uses": ["8:24-8:25|4|-1"]
    }]
}
*/
