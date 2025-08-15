
template <class T> struct type_trait {
  const static bool value = false;
};

template <> struct type_trait<int> {
  const static bool value = true;
};

template <class T>
concept Con1 = type_trait<T>::value;

constexpr int sizeFunc() { return 4; }

template <class T>
concept ConWithLogicalAnd = Con1<T> && sizeof(T) > sizeFunc();

namespace ns {
template <class T>
concept ConInNamespace = sizeof(T) > 4;
}

template <class T1, class T2>
concept ConTwoTemplateParams = ns::ConInNamespace<T1> && ConWithLogicalAnd<T2>;

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3226866773869731400,
      "detailed_name": "constexpr int sizeFunc()",
      "qual_name_offset": 14,
      "short_name": "sizeFunc",
      "spell": "13:15-13:23|13:1-13:39|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["16:52-16:60|36|-1"]
    }],
  "usr2type": [{
      "usr": 436,
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
      "instances": [3643386974125063532, 4683419091429829178],
      "uses": []
    }, {
      "usr": 452,
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
      "instances": [15611304989063975809, 15197037962155352994, 7075924720131397743, 8419381068906673567],
      "uses": []
    }, {
      "usr": 368613743269466510,
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
      "declarations": ["19:17-19:18|19:11-19:18|1025|-1"],
      "derived": [],
      "instances": [],
      "uses": ["20:33-20:34|4|-1"]
    }, {
      "usr": 1341599025369786548,
      "detailed_name": "T1",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 0,
      "declarations": ["23:17-23:19|23:11-23:19|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["24:51-24:53|4|-1"]
    }, {
      "usr": 1789177110200181456,
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
      "declarations": ["15:17-15:18|15:11-15:18|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["16:34-16:35|4|-1", "16:47-16:48|4|-1"]
    }, {
      "usr": 4001289545226345448,
      "detailed_name": "struct type_trait {}",
      "qual_name_offset": 7,
      "short_name": "type_trait",
      "spell": "2:27-2:37|2:20-4:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [13813325012676356715],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 1,
      "declarations": [],
      "derived": [10139416838611429657],
      "instances": [],
      "uses": ["11:16-11:26|4|-1"]
    }, {
      "usr": 8987540007709901036,
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
      "declarations": ["10:17-10:18|10:11-10:18|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["11:27-11:28|4|-1"]
    }, {
      "usr": 10139416838611429657,
      "detailed_name": "template<> struct type_trait<int> {}",
      "qual_name_offset": 18,
      "short_name": "type_trait",
      "spell": "6:20-6:30|6:1-8:2|2|-1",
      "bases": [4001289545226345448],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 10306412732558468540,
      "detailed_name": "T2",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 0,
      "declarations": ["23:27-23:29|23:21-23:29|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["24:76-24:78|4|-1"]
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "namespace ns {}",
      "qual_name_offset": 10,
      "short_name": "ns",
      "bases": [],
      "funcs": [],
      "types": [368613743269466510],
      "vars": [{
          "L": 7075924720131397743,
          "R": -1
        }],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["18:11-18:13|18:1-21:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["24:32-24:34|4|-1"]
    }, {
      "usr": 13813325012676356715,
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
      "declarations": ["2:17-2:18|2:11-2:18|1025|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 3643386974125063532,
      "detailed_name": "static const bool type_trait::value",
      "qual_name_offset": 18,
      "short_name": "value",
      "hover": "static const bool type_trait::value = false",
      "type": 436,
      "kind": 8,
      "parent_kind": 23,
      "storage": 2,
      "declarations": ["3:21-3:26|3:3-3:34|1025|-1"],
      "uses": ["11:31-11:36|4|-1"]
    }, {
      "usr": 4683419091429829178,
      "detailed_name": "static const bool type_trait<int>::value",
      "qual_name_offset": 18,
      "short_name": "value",
      "hover": "static const bool type_trait<int>::value = true",
      "type": 436,
      "kind": 8,
      "parent_kind": 5,
      "storage": 2,
      "declarations": ["7:21-7:26|7:3-7:33|1025|-1"],
      "uses": []
    }, {
      "usr": 7075924720131397743,
      "detailed_name": "int ns::ConInNamespace",
      "qual_name_offset": 4,
      "short_name": "ConInNamespace",
      "hover": "int ns::ConInNamespace = sizeof(T) > 4",
      "spell": "20:9-20:23|20:1-20:39|1026|-1",
      "type": 452,
      "kind": 13,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "uses": ["24:36-24:50|4|-1"]
    }, {
      "usr": 8419381068906673567,
      "detailed_name": "int ConTwoTemplateParams",
      "qual_name_offset": 4,
      "short_name": "ConTwoTemplateParams",
      "hover": "int ConTwoTemplateParams = ns::ConInNamespace<T1> && ConWithLogicalAnd<T2>",
      "spell": "24:9-24:29|24:1-24:79|2|-1",
      "type": 452,
      "kind": 13,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 15197037962155352994,
      "detailed_name": "int ConWithLogicalAnd",
      "qual_name_offset": 4,
      "short_name": "ConWithLogicalAnd",
      "hover": "int ConWithLogicalAnd = Con1<T> && sizeof(T) > sizeFunc()",
      "spell": "16:9-16:26|16:1-16:62|2|-1",
      "type": 452,
      "kind": 13,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": ["24:58-24:75|4|-1"]
    }, {
      "usr": 15611304989063975809,
      "detailed_name": "int Con1",
      "qual_name_offset": 4,
      "short_name": "Con1",
      "hover": "int Con1 = type_trait<T>::value",
      "spell": "11:9-11:13|11:1-11:36|2|-1",
      "type": 452,
      "kind": 13,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": ["16:29-16:33|4|-1"]
    }]
}
*/
