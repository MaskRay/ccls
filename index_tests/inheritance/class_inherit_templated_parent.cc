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
      "usr": 5863733211528032190,
      "detailed_name": "class Derived1 : Base1<T> {}",
      "qual_name_offset": 6,
      "short_name": "Derived1",
      "kind": 5,
      "declarations": [],
      "spell": "8:7-8:15|0|1|2",
      "extent": "8:1-8:29|0|1|0",
      "alias_of": 0,
      "bases": [11930058224338108382],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:43-13:51|10963370434658308541|2|2052"]
    }, {
      "usr": 10651399730831737929,
      "detailed_name": "class Derived2 : Base2<T> {}",
      "qual_name_offset": 6,
      "short_name": "Derived2",
      "kind": 5,
      "declarations": [],
      "spell": "11:7-11:15|0|1|2",
      "extent": "11:1-11:29|0|1|0",
      "alias_of": 0,
      "bases": [11118288764693061434],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:56-13:64|10963370434658308541|2|2052"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "class Derived : Base1<3>, Base2<Derived>, Derived1<4>, Derived2<Derived> {}",
      "qual_name_offset": 6,
      "short_name": "Derived",
      "kind": 5,
      "declarations": [],
      "spell": "13:7-13:14|0|1|2",
      "extent": "13:1-13:76|0|1|0",
      "alias_of": 0,
      "bases": [11930058224338108382, 11118288764693061434, 5863733211528032190, 10651399730831737929],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:33-13:40|10963370434658308541|2|2052", "13:65-13:72|10963370434658308541|2|2052"]
    }, {
      "usr": 11118288764693061434,
      "detailed_name": "class Base2 {}",
      "qual_name_offset": 6,
      "short_name": "Base2",
      "kind": 5,
      "declarations": [],
      "spell": "5:7-5:12|0|1|2",
      "extent": "5:1-5:15|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [10651399730831737929, 10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:27-13:32|10963370434658308541|2|2052"]
    }, {
      "usr": 11930058224338108382,
      "detailed_name": "class Base1 {}",
      "qual_name_offset": 6,
      "short_name": "Base1",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:12|0|1|2",
      "extent": "2:1-2:15|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [5863733211528032190, 10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:17-13:22|10963370434658308541|2|2052"]
    }],
  "usr2var": []
}
*/
