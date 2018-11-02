template<class T>
class function;

template<typename T, typename... Args>
class function<T(Args...)> {};

function<void(int)> f;

template<typename T> class allocator;

template<typename T, typename Alloc = allocator<T> >
class vector {
  void clear();
};

template<typename T>
class vector<T*> {};

struct Z1 {};

template class vector<Z1>;

struct Z2 {};

template<>
class vector<Z2> {
  void clear();
};

vector<char> vc;
vector<int*> vip;
vector<Z1> vz1;
vector<Z2> vz2;

enum Enum {
  Enum0, Enum1
};
template <typename T, int I, Enum, int E>
void foo(T Value) {}

static const int kOnst = 7;
template <>
void foo<float, 9, Enum0, kOnst + 7>(float Value);

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3861597222587452538,
      "detailed_name": "template<> void foo<float, 9, Enum0, 14>(float Value)",
      "qual_name_offset": 16,
      "short_name": "foo",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["43:6-43:9|42:1-43:50|1|-1"],
      "derived": [],
      "uses": []
    }, {
      "usr": 6113470698424012876,
      "detailed_name": "void vector<Z2, allocator<Z2> >::clear()",
      "qual_name_offset": 5,
      "short_name": "clear",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["27:8-27:13|27:3-27:15|1025|-1"],
      "derived": [],
      "uses": []
    }, {
      "usr": 17498190318698490707,
      "detailed_name": "void foo(T Value)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "39:6-39:9|39:1-39:21|2|-1",
      "bases": [],
      "vars": [17826688417349629938],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 18107614608385228556,
      "detailed_name": "void vector::clear()",
      "qual_name_offset": 5,
      "short_name": "clear",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["13:8-13:13|13:3-13:15|1025|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 53,
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
      "instances": [13914496963221806870],
      "uses": []
    }, {
      "usr": 218068462278884837,
      "detailed_name": "template <typename T, typename ...Args> class function<type-parameter-0-0 (type-parameter-0-1...)> {}",
      "qual_name_offset": 46,
      "short_name": "function",
      "spell": "5:7-5:15|4:1-5:30|2|-1",
      "bases": [15019211479263750068],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [2933643612409209903],
      "uses": ["7:1-7:9|4|-1"]
    }, {
      "usr": 1663022413889915338,
      "detailed_name": "template<> class vector<Z2, allocator<Z2>> {}",
      "qual_name_offset": 17,
      "short_name": "vector",
      "spell": "26:7-26:13|25:1-28:2|2|-1",
      "bases": [7440942986741176606],
      "funcs": [6113470698424012876],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [15931696253641284761],
      "uses": ["26:7-26:13|4|-1", "33:1-33:7|4|-1"]
    }, {
      "usr": 5760043510674081814,
      "detailed_name": "struct Z1 {}",
      "qual_name_offset": 7,
      "short_name": "Z1",
      "spell": "19:8-19:10|19:1-19:13|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["21:23-21:25|4|-1", "32:8-32:10|4|-1"]
    }, {
      "usr": 7440942986741176606,
      "detailed_name": "class vector {}",
      "qual_name_offset": 6,
      "short_name": "vector",
      "spell": "12:7-12:13|12:1-14:2|2|-1",
      "bases": [],
      "funcs": [18107614608385228556],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [16155717907537731864, 1663022413889915338],
      "instances": [5792869548777559988],
      "uses": ["17:7-17:13|4|-1", "21:16-21:22|4|-1", "30:1-30:7|4|-1", "32:1-32:7|4|-1"]
    }, {
      "usr": 9201299975592934124,
      "detailed_name": "enum Enum {}",
      "qual_name_offset": 5,
      "short_name": "Enum",
      "spell": "35:6-35:10|35:1-37:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 10124869160135436852,
      "detailed_name": "struct Z2 {}",
      "qual_name_offset": 7,
      "short_name": "Z2",
      "spell": "23:8-23:10|23:1-23:13|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["26:14-26:16|4|-1", "33:8-33:10|4|-1"]
    }, {
      "usr": 14111105212951082474,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "spell": "38:20-38:21|38:11-38:21|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 5,
      "declarations": [],
      "derived": [],
      "instances": [17826688417349629938],
      "uses": []
    }, {
      "usr": 15019211479263750068,
      "detailed_name": "class function",
      "qual_name_offset": 6,
      "short_name": "function",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": ["2:7-2:15|2:1-2:15|1|-1"],
      "derived": [218068462278884837],
      "instances": [],
      "uses": ["5:7-5:15|4|-1"]
    }, {
      "usr": 15440970074034693939,
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
      "instances": [3566687051827176322],
      "uses": []
    }, {
      "usr": 15695704394170757108,
      "detailed_name": "class allocator",
      "qual_name_offset": 6,
      "short_name": "allocator",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": ["9:28-9:37|9:22-9:37|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["11:39-11:48|4|-1"]
    }, {
      "usr": 16155717907537731864,
      "detailed_name": "template <typename T> class vector<type-parameter-0-0 *, allocator<type-parameter-0-0 *>> {}",
      "qual_name_offset": 28,
      "short_name": "vector",
      "spell": "17:7-17:13|16:1-17:20|2|-1",
      "bases": [7440942986741176606],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [86949563628772958],
      "uses": ["31:1-31:7|4|-1"]
    }],
  "usr2var": [{
      "usr": 86949563628772958,
      "detailed_name": "vector<int *> vip",
      "qual_name_offset": 14,
      "short_name": "vip",
      "spell": "31:14-31:17|31:1-31:17|2|-1",
      "type": 16155717907537731864,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 2933643612409209903,
      "detailed_name": "function<void (int)> f",
      "qual_name_offset": 21,
      "short_name": "f",
      "spell": "7:21-7:22|7:1-7:22|2|-1",
      "type": 218068462278884837,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 3566687051827176322,
      "detailed_name": "vector<Z1> vz1",
      "qual_name_offset": 11,
      "short_name": "vz1",
      "spell": "32:12-32:15|32:1-32:15|2|-1",
      "type": 15440970074034693939,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 4917621020431490070,
      "detailed_name": "Enum1",
      "qual_name_offset": 0,
      "short_name": "Enum1",
      "hover": "Enum1 = 1",
      "spell": "36:10-36:15|36:10-36:15|1026|-1",
      "type": 9201299975592934124,
      "kind": 22,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 5792869548777559988,
      "detailed_name": "vector<char> vc",
      "qual_name_offset": 13,
      "short_name": "vc",
      "spell": "30:14-30:16|30:1-30:16|2|-1",
      "type": 7440942986741176606,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 13914496963221806870,
      "detailed_name": "static const int kOnst",
      "qual_name_offset": 17,
      "short_name": "kOnst",
      "hover": "static const int kOnst = 7",
      "spell": "41:18-41:23|41:1-41:27|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 2,
      "declarations": [],
      "uses": ["43:27-43:32|12|-1"]
    }, {
      "usr": 15477793821005285152,
      "detailed_name": "Enum0",
      "qual_name_offset": 0,
      "short_name": "Enum0",
      "hover": "Enum0 = 0",
      "spell": "36:3-36:8|36:3-36:8|1026|-1",
      "type": 9201299975592934124,
      "kind": 22,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": ["43:20-43:25|4|-1"]
    }, {
      "usr": 15931696253641284761,
      "detailed_name": "vector<Z2> vz2",
      "qual_name_offset": 11,
      "short_name": "vz2",
      "spell": "33:12-33:15|33:1-33:15|2|-1",
      "type": 1663022413889915338,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 17826688417349629938,
      "detailed_name": "T Value",
      "qual_name_offset": 2,
      "short_name": "Value",
      "spell": "39:12-39:17|39:10-39:17|1026|-1",
      "type": 14111105212951082474,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
