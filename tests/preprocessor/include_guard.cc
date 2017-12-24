#ifndef FOO
#define FOO

#endif

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:include_guard.cc@21@macro@FOO",
      "short_name": "FOO",
      "detailed_name": "FOO",
      "hover": "#define FOO",
      "definition_spelling": "2:9-2:12",
      "definition_extent": "2:9-2:12",
      "cls": 2,
      "uses": ["2:9-2:12"]
    }]
}
*/
