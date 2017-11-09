#include "make_functions.h"

template <typename T, typename... Args>
T* MakeUnique(Args&&... args) {
  return nullptr;
}

template <typename T, typename... Args>
T* maKE_NoRefs(Args... args) {
  return nullptr;
}

void caller22() {
  MakeUnique<Foobar>();
  MakeUnique<Foobar>(1);
  MakeUnique<Foobar>(1, new Bar(), nullptr);
  maKE_NoRefs<Foobar>(1, new Bar(), nullptr);
}

// TODO: Eliminate the extra entries in the "types" array here. They come from
// the template function definitions.

// Foobar is defined in a separate file to ensure that we can attribute
// MakeUnique calls across translation units.

/*
OUTPUT: make_functions.h
{
  "dependencies": ["C:/Users/jacob/Desktop/cquery/tests/constructors/make_functions.cc"],
  "types": [{
      "id": 0,
      "usr": "c:@S@Bar",
      "short_name": "Bar",
      "detailed_name": "Bar",
      "definition_spelling": "1:8-1:11",
      "definition_extent": "1:1-1:14",
      "uses": ["1:8-1:11", "7:17-7:20", "8:15-8:18"]
    }, {
      "id": 1,
      "usr": "c:@S@Foobar",
      "short_name": "Foobar",
      "detailed_name": "Foobar",
      "definition_spelling": "3:7-3:13",
      "definition_extent": "3:1-9:2",
      "funcs": [0, 1, 2, 3],
      "uses": ["3:7-3:13", "5:3-5:9", "6:3-6:9", "7:3-7:9", "8:3-8:9"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foobar@F@Foobar#",
      "short_name": "Foobar",
      "detailed_name": "void Foobar::Foobar()",
      "is_constructor": true,
      "definition_spelling": "5:3-5:9",
      "definition_extent": "5:3-5:14",
      "declaring_type": 1
    }, {
      "id": 1,
      "usr": "c:@S@Foobar@F@Foobar#I#",
      "short_name": "Foobar",
      "detailed_name": "void Foobar::Foobar(int)",
      "is_constructor": true,
      "parameter_type_descriptions": ["int"],
      "definition_spelling": "6:3-6:9",
      "definition_extent": "6:3-6:17",
      "declaring_type": 1
    }, {
      "id": 2,
      "usr": "c:@S@Foobar@F@Foobar#&&I#*$@S@Bar#*b#",
      "short_name": "Foobar",
      "detailed_name": "void Foobar::Foobar(int &&, Bar *, bool *)",
      "is_constructor": true,
      "parameter_type_descriptions": ["int &&", "Bar *", "bool *"],
      "definition_spelling": "7:3-7:9",
      "definition_extent": "7:3-7:32",
      "declaring_type": 1
    }, {
      "id": 3,
      "usr": "c:@S@Foobar@F@Foobar#I#*$@S@Bar#*b#",
      "short_name": "Foobar",
      "detailed_name": "void Foobar::Foobar(int, Bar *, bool *)",
      "is_constructor": true,
      "parameter_type_descriptions": ["int", "Bar *", "bool *"],
      "definition_spelling": "8:3-8:9",
      "definition_extent": "8:3-8:30",
      "declaring_type": 1
    }]
}
OUTPUT: make_functions.cc
{
  "includes": [{
      "line": 1,
      "resolved_path": "C:/Users/jacob/Desktop/cquery/tests/constructors/make_functions.h"
    }],
  "dependencies": ["C:/Users/jacob/Desktop/cquery/tests/constructors/make_functions.h"],
  "types": [{
      "id": 0,
      "usr": "c:make_functions.cc@41",
      "uses": ["4:1-4:2"]
    }, {
      "id": 1,
      "usr": "c:make_functions.cc@53",
      "uses": ["4:15-4:19"]
    }, {
      "id": 2,
      "usr": "c:make_functions.cc@139",
      "uses": ["9:1-9:2"]
    }, {
      "id": 3,
      "usr": "c:make_functions.cc@151",
      "uses": ["9:16-9:20"]
    }, {
      "id": 4,
      "usr": "c:@S@Foobar",
      "uses": ["14:14-14:20", "15:14-15:20", "16:14-16:20", "17:15-17:21"]
    }, {
      "id": 5,
      "usr": "c:@S@Bar",
      "uses": ["16:29-16:32", "17:30-17:33"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@FT@>2#T#pTMakeUnique#P&&t0.1#*t0.0#",
      "short_name": "MakeUnique",
      "detailed_name": "T *MakeUnique(Args &&...)",
      "is_constructor": false,
      "parameter_type_descriptions": ["Args &&..."],
      "definition_spelling": "4:4-4:14",
      "definition_extent": "4:1-6:2",
      "callers": ["2@14:3-14:13", "2@15:3-15:13", "2@16:3-16:13"]
    }, {
      "id": 1,
      "usr": "c:@FT@>2#T#pTmaKE_NoRefs#Pt0.1#*t0.0#",
      "short_name": "maKE_NoRefs",
      "detailed_name": "T *maKE_NoRefs(Args...)",
      "is_constructor": false,
      "parameter_type_descriptions": ["Args..."],
      "definition_spelling": "9:4-9:15",
      "definition_extent": "9:1-11:2",
      "callers": ["2@17:3-17:14"]
    }, {
      "id": 2,
      "usr": "c:@F@caller22#",
      "short_name": "caller22",
      "detailed_name": "void caller22()",
      "is_constructor": false,
      "definition_spelling": "13:6-13:14",
      "definition_extent": "13:1-18:2",
      "callees": ["0@14:3-14:13", "0@15:3-15:13", "0@16:3-16:13", "1@17:3-17:14"]
    }, {
      "id": 3,
      "usr": "c:@S@Foobar@F@Foobar#",
      "is_constructor": false,
      "callers": ["~-1@14:3-14:13"]
    }, {
      "id": 4,
      "usr": "c:@S@Foobar@F@Foobar#I#",
      "is_constructor": false,
      "callers": ["~-1@15:3-15:13"]
    }, {
      "id": 5,
      "usr": "c:@S@Foobar@F@Foobar#&&I#*$@S@Bar#*b#",
      "is_constructor": false,
      "callers": ["~-1@16:3-16:13"]
    }, {
      "id": 6,
      "usr": "c:@S@Foobar@F@Foobar#I#*$@S@Bar#*b#",
      "is_constructor": false,
      "callers": ["~-1@17:3-17:14"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:make_functions.cc@86@FT@>2#T#pTMakeUnique#P&&t0.1#*t0.0#@args",
      "short_name": "args",
      "detailed_name": "Args &&... args",
      "definition_spelling": "4:25-4:29",
      "definition_extent": "4:15-4:29",
      "is_local": true,
      "is_macro": false,
      "uses": ["4:25-4:29"]
    }, {
      "id": 1,
      "usr": "c:make_functions.cc@185@FT@>2#T#pTmaKE_NoRefs#Pt0.1#*t0.0#@args",
      "short_name": "args",
      "detailed_name": "Args... args",
      "definition_spelling": "9:24-9:28",
      "definition_extent": "9:16-9:28",
      "is_local": true,
      "is_macro": false,
      "uses": ["9:24-9:28"]
    }]
}
*/
