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
      "kind": 12,
      "storage": 0,
      "declarations": ["43:6-43:9|0|1|1"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 6113470698424012876,
      "detailed_name": "void vector<Z2, allocator<Z2> >::clear()",
      "qual_name_offset": 5,
      "short_name": "clear",
      "kind": 6,
      "storage": 0,
      "declarations": ["27:8-27:13|1663022413889915338|2|1025"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 17498190318698490707,
      "detailed_name": "void foo(T Value)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "39:6-39:9|0|1|2",
      "extent": "39:1-39:21|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [17826688417349629938],
      "uses": [],
      "callees": []
    }, {
      "usr": 18107614608385228556,
      "detailed_name": "void vector::clear()",
      "qual_name_offset": 5,
      "short_name": "clear",
      "kind": 6,
      "storage": 0,
      "declarations": ["13:8-13:13|7440942986741176606|2|1025"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 53,
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
      "instances": [13914496963221806870],
      "uses": []
    }, {
      "usr": 218068462278884837,
      "detailed_name": "template <typename T, typename ...Args> class function<type-parameter-0-0 (type-parameter-0-1...)> {}",
      "qual_name_offset": 46,
      "short_name": "function",
      "kind": 5,
      "declarations": [],
      "spell": "5:7-5:15|0|1|2",
      "extent": "4:1-5:30|0|1|0",
      "alias_of": 0,
      "bases": [15019211479263750068],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:1-7:9|0|1|4"]
    }, {
      "usr": 1663022413889915338,
      "detailed_name": "template<> class vector<Z2, allocator<Z2>> {}",
      "qual_name_offset": 17,
      "short_name": "vector",
      "kind": 5,
      "declarations": [],
      "spell": "26:7-26:13|0|1|2",
      "extent": "25:1-28:2|0|1|0",
      "alias_of": 0,
      "bases": [7440942986741176606],
      "derived": [],
      "types": [],
      "funcs": [6113470698424012876],
      "vars": [],
      "instances": [15931696253641284761],
      "uses": ["26:7-26:13|0|1|4", "33:1-33:7|0|1|4"]
    }, {
      "usr": 3231449734830406187,
      "detailed_name": "function",
      "qual_name_offset": 0,
      "short_name": "function",
      "kind": 26,
      "declarations": [],
      "spell": "5:7-5:15|0|1|2",
      "extent": "4:1-5:30|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2933643612409209903],
      "uses": []
    }, {
      "usr": 5760043510674081814,
      "detailed_name": "struct Z1 {}",
      "qual_name_offset": 7,
      "short_name": "Z1",
      "kind": 23,
      "declarations": [],
      "spell": "19:8-19:10|0|1|2",
      "extent": "19:1-19:13|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["21:23-21:25|0|1|4", "32:8-32:10|0|1|4"]
    }, {
      "usr": 7440942986741176606,
      "detailed_name": "class vector {}",
      "qual_name_offset": 6,
      "short_name": "vector",
      "kind": 5,
      "declarations": [],
      "spell": "12:7-12:13|0|1|2",
      "extent": "12:1-14:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [16155717907537731864, 1663022413889915338],
      "types": [],
      "funcs": [18107614608385228556],
      "vars": [],
      "instances": [],
      "uses": ["21:16-21:22|0|1|4", "30:1-30:7|0|1|4", "32:1-32:7|0|1|4"]
    }, {
      "usr": 9201299975592934124,
      "detailed_name": "enum Enum {\n}",
      "qual_name_offset": 5,
      "short_name": "Enum",
      "kind": 10,
      "declarations": [],
      "spell": "35:6-35:10|0|1|2",
      "extent": "35:1-37:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 10124869160135436852,
      "detailed_name": "struct Z2 {}",
      "qual_name_offset": 7,
      "short_name": "Z2",
      "kind": 23,
      "declarations": [],
      "spell": "23:8-23:10|0|1|2",
      "extent": "23:1-23:13|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["26:14-26:16|0|1|4", "33:8-33:10|0|1|4"]
    }, {
      "usr": 11153492883079050853,
      "detailed_name": "vector",
      "qual_name_offset": 0,
      "short_name": "vector",
      "kind": 26,
      "declarations": [],
      "spell": "17:7-17:13|0|1|2",
      "extent": "16:1-17:20|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [86949563628772958],
      "uses": []
    }, {
      "usr": 13322943937025195708,
      "detailed_name": "vector",
      "qual_name_offset": 0,
      "short_name": "vector",
      "kind": 26,
      "declarations": [],
      "spell": "12:7-12:13|0|1|2",
      "extent": "11:1-14:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [5792869548777559988],
      "uses": []
    }, {
      "usr": 14111105212951082474,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "38:20-38:21|17498190318698490707|3|2",
      "extent": "38:11-38:21|17498190318698490707|3|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [17826688417349629938],
      "uses": []
    }, {
      "usr": 15019211479263750068,
      "detailed_name": "class function",
      "qual_name_offset": 6,
      "short_name": "function",
      "kind": 5,
      "declarations": ["2:7-2:15|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [218068462278884837],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 15440970074034693939,
      "detailed_name": "vector",
      "qual_name_offset": 0,
      "short_name": "vector",
      "kind": 26,
      "declarations": [],
      "spell": "21:16-21:22|0|1|2",
      "extent": "21:1-21:26|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [3566687051827176322],
      "uses": []
    }, {
      "usr": 15695704394170757108,
      "detailed_name": "class allocator",
      "qual_name_offset": 6,
      "short_name": "allocator",
      "kind": 5,
      "declarations": ["9:28-9:37|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 16155717907537731864,
      "detailed_name": "template <typename T> class vector<type-parameter-0-0 *, allocator<type-parameter-0-0 *>> {}",
      "qual_name_offset": 28,
      "short_name": "vector",
      "kind": 5,
      "declarations": [],
      "spell": "17:7-17:13|0|1|2",
      "extent": "16:1-17:20|0|1|0",
      "alias_of": 0,
      "bases": [7440942986741176606],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["31:1-31:7|0|1|4"]
    }],
  "usr2var": [{
      "usr": 86949563628772958,
      "detailed_name": "vector<int *> vip",
      "qual_name_offset": 14,
      "short_name": "vip",
      "declarations": [],
      "spell": "31:14-31:17|0|1|2",
      "extent": "31:1-31:17|0|1|0",
      "type": 11153492883079050853,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 2933643612409209903,
      "detailed_name": "function<void (int)> f",
      "qual_name_offset": 0,
      "short_name": "f",
      "declarations": [],
      "spell": "7:21-7:22|0|1|2",
      "extent": "7:1-7:22|0|1|0",
      "type": 3231449734830406187,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 3566687051827176322,
      "detailed_name": "vector<Z1> vz1",
      "qual_name_offset": 11,
      "short_name": "vz1",
      "declarations": [],
      "spell": "32:12-32:15|0|1|2",
      "extent": "32:1-32:15|0|1|0",
      "type": 15440970074034693939,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 4917621020431490070,
      "detailed_name": "Enum1",
      "qual_name_offset": 0,
      "short_name": "Enum1",
      "hover": "Enum1 = 1",
      "declarations": [],
      "spell": "36:10-36:15|9201299975592934124|2|1026",
      "extent": "36:10-36:15|9201299975592934124|2|0",
      "type": 0,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "usr": 5792869548777559988,
      "detailed_name": "vector<char> vc",
      "qual_name_offset": 13,
      "short_name": "vc",
      "declarations": [],
      "spell": "30:14-30:16|0|1|2",
      "extent": "30:1-30:16|0|1|0",
      "type": 13322943937025195708,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 13914496963221806870,
      "detailed_name": "static const int kOnst",
      "qual_name_offset": 17,
      "short_name": "kOnst",
      "hover": "static const int kOnst = 7",
      "declarations": [],
      "spell": "41:18-41:23|0|1|2",
      "extent": "41:1-41:27|0|1|0",
      "type": 53,
      "uses": ["43:27-43:32|0|1|12"],
      "kind": 13,
      "storage": 2
    }, {
      "usr": 15477793821005285152,
      "detailed_name": "Enum0",
      "qual_name_offset": 0,
      "short_name": "Enum0",
      "hover": "Enum0 = 0",
      "declarations": [],
      "spell": "36:3-36:8|9201299975592934124|2|1026",
      "extent": "36:3-36:8|9201299975592934124|2|0",
      "type": 0,
      "uses": ["43:20-43:25|9201299975592934124|2|4"],
      "kind": 22,
      "storage": 0
    }, {
      "usr": 15931696253641284761,
      "detailed_name": "vector<Z2> vz2",
      "qual_name_offset": 11,
      "short_name": "vz2",
      "declarations": [],
      "spell": "33:12-33:15|0|1|2",
      "extent": "33:1-33:15|0|1|0",
      "type": 1663022413889915338,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 17826688417349629938,
      "detailed_name": "T Value",
      "qual_name_offset": 2,
      "short_name": "Value",
      "declarations": [],
      "spell": "39:12-39:17|17498190318698490707|3|1026",
      "extent": "39:10-39:17|17498190318698490707|3|0",
      "type": 14111105212951082474,
      "uses": [],
      "kind": 253,
      "storage": 0
    }]
}
*/
