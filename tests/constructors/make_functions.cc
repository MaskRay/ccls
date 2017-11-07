template <typename T, typename... Args>
T* MakeUnique(Args&&... args) {
  return nullptr;
}

template <typename T, typename... Args>
T* maKE_NoRefs(Args... args) {
  return nullptr;
}

struct Bar {};
class Foobar {
 public:
  Foobar() {}
  Foobar(int) {}
  Foobar(int&&, Bar*, bool*) {}
  Foobar(int, Bar*, bool*) {}
};
void caller22() {
  MakeUnique<Foobar>();
  MakeUnique<Foobar>(1);
  MakeUnique<Foobar>(1, new Bar(), nullptr);
  maKE_NoRefs<Foobar>(1, new Bar(), nullptr);
}

// TODO: Eliminate the extra entries in the "types" array here. They come from
// the template function definitions.

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:make_functions.cc@10",
      "uses": ["2:1-2:2"]
    }, {
      "id": 1,
      "usr": "c:make_functions.cc@22",
      "uses": ["2:15-2:19"]
    }, {
      "id": 2,
      "usr": "c:make_functions.cc@108",
      "uses": ["7:1-7:2"]
    }, {
      "id": 3,
      "usr": "c:make_functions.cc@120",
      "uses": ["7:16-7:20"]
    }, {
      "id": 4,
      "usr": "c:@S@Bar",
      "short_name": "Bar",
      "detailed_name": "Bar",
      "definition_spelling": "11:8-11:11",
      "definition_extent": "11:1-11:14",
      "uses": ["11:8-11:11", "16:17-16:20", "17:15-17:18", "22:29-22:32", "23:30-23:33"]
    }, {
      "id": 5,
      "usr": "c:@S@Foobar",
      "short_name": "Foobar",
      "detailed_name": "Foobar",
      "definition_spelling": "12:7-12:13",
      "definition_extent": "12:1-18:2",
      "funcs": [2, 3, 4, 5],
      "uses": ["12:7-12:13", "14:3-14:9", "15:3-15:9", "16:3-16:9", "17:3-17:9", "20:14-20:20", "21:14-21:20", "22:14-22:20", "23:15-23:21"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@FT@>2#T#pTMakeUnique#P&&t0.1#*t0.0#",
      "short_name": "MakeUnique",
      "detailed_name": "T *MakeUnique(Args &&...)",
      "is_constructor": false,
      "parameter_type_descriptions": ["Args &&..."],
      "definition_spelling": "2:4-2:14",
      "definition_extent": "2:1-4:2",
      "callers": ["6@20:3-20:13", "6@21:3-21:13", "6@22:3-22:13"]
    }, {
      "id": 1,
      "usr": "c:@FT@>2#T#pTmaKE_NoRefs#Pt0.1#*t0.0#",
      "short_name": "maKE_NoRefs",
      "detailed_name": "T *maKE_NoRefs(Args...)",
      "is_constructor": false,
      "parameter_type_descriptions": ["Args..."],
      "definition_spelling": "7:4-7:15",
      "definition_extent": "7:1-9:2",
      "callers": ["6@23:3-23:14"]
    }, {
      "id": 2,
      "usr": "c:@S@Foobar@F@Foobar#",
      "short_name": "Foobar",
      "detailed_name": "void Foobar::Foobar()",
      "is_constructor": true,
      "definition_spelling": "14:3-14:9",
      "definition_extent": "14:3-14:14",
      "declaring_type": 5
    }, {
      "id": 3,
      "usr": "c:@S@Foobar@F@Foobar#I#",
      "short_name": "Foobar",
      "detailed_name": "void Foobar::Foobar(int)",
      "is_constructor": true,
      "parameter_type_descriptions": ["int"],
      "definition_spelling": "15:3-15:9",
      "definition_extent": "15:3-15:17",
      "declaring_type": 5
    }, {
      "id": 4,
      "usr": "c:@S@Foobar@F@Foobar#&&I#*$@S@Bar#*b#",
      "short_name": "Foobar",
      "detailed_name": "void Foobar::Foobar(int &&, Bar *, bool *)",
      "is_constructor": true,
      "parameter_type_descriptions": ["int &&", "Bar *", "bool *"],
      "definition_spelling": "16:3-16:9",
      "definition_extent": "16:3-16:32",
      "declaring_type": 5
    }, {
      "id": 5,
      "usr": "c:@S@Foobar@F@Foobar#I#*$@S@Bar#*b#",
      "short_name": "Foobar",
      "detailed_name": "void Foobar::Foobar(int, Bar *, bool *)",
      "is_constructor": true,
      "parameter_type_descriptions": ["int", "Bar *", "bool *"],
      "definition_spelling": "17:3-17:9",
      "definition_extent": "17:3-17:30",
      "declaring_type": 5
    }, {
      "id": 6,
      "usr": "c:@F@caller22#",
      "short_name": "caller22",
      "detailed_name": "void caller22()",
      "is_constructor": false,
      "definition_spelling": "19:6-19:14",
      "definition_extent": "19:1-24:2",
      "callees": ["0@20:3-20:13", "0@21:3-21:13", "0@22:3-22:13", "1@23:3-23:14"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:make_functions.cc@55@FT@>2#T#pTMakeUnique#P&&t0.1#*t0.0#@args",
      "short_name": "args",
      "detailed_name": "Args &&... args",
      "definition_spelling": "2:25-2:29",
      "definition_extent": "2:15-2:29",
      "is_local": true,
      "is_macro": false,
      "uses": ["2:25-2:29"]
    }, {
      "id": 1,
      "usr": "c:make_functions.cc@154@FT@>2#T#pTmaKE_NoRefs#Pt0.1#*t0.0#@args",
      "short_name": "args",
      "detailed_name": "Args... args",
      "definition_spelling": "7:24-7:28",
      "definition_extent": "7:16-7:28",
      "is_local": true,
      "is_macro": false,
      "uses": ["7:24-7:28"]
    }]
}
*/
