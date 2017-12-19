#pragma once

#include <string>
#include <vector>

struct CompilationEntry {
  std::string directory;
  std::string filename;
  std::vector<std::string> args;
};

std::vector<CompilationEntry> LoadCompilationEntriesFromDirectory(const std::string& project_directory);

/*
OUTPUT:
{
  "includes": [{
      "line": 3,
      "resolved_path": "&string"
    }, {
      "line": 4,
      "resolved_path": "&vector"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@CompilationEntry",
      "short_name": "CompilationEntry",
      "detailed_name": "CompilationEntry",
      "definition_spelling": "6:8-6:24",
      "definition_extent": "6:1-10:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1, 2],
      "instances": [],
      "uses": ["6:8-6:24", "12:13-12:29"]
    }, {
      "id": 1,
      "usr": "c:@N@std@T@string",
      "short_name": "",
      "detailed_name": "",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": ["7:8-7:14", "8:8-8:14", "9:20-9:26", "12:78-12:84"]
    }, {
      "id": 2,
      "usr": "c:@N@std@ST>2#T#T@vector",
      "short_name": "",
      "detailed_name": "",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2],
      "uses": ["9:8-9:14", "12:6-12:12"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@LoadCompilationEntriesFromDirectory#&1$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#",
      "short_name": "LoadCompilationEntriesFromDirectory",
      "detailed_name": "std::vector<CompilationEntry> LoadCompilationEntriesFromDirectory(const std::string &)",
      "declarations": [{
          "spelling": "12:31-12:66",
          "extent": "12:1-12:104",
          "content": "std::vector<CompilationEntry> LoadCompilationEntriesFromDirectory(const std::string& project_directory)",
          "param_spellings": ["12:86-12:103"]
        }],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@CompilationEntry@FI@directory",
      "short_name": "directory",
      "detailed_name": "std::string CompilationEntry::directory",
      "definition_spelling": "7:15-7:24",
      "definition_extent": "7:3-7:24",
      "variable_type": 1,
      "declaring_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["7:15-7:24"]
    }, {
      "id": 1,
      "usr": "c:@S@CompilationEntry@FI@filename",
      "short_name": "filename",
      "detailed_name": "std::string CompilationEntry::filename",
      "definition_spelling": "8:15-8:23",
      "definition_extent": "8:3-8:23",
      "variable_type": 1,
      "declaring_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["8:15-8:23"]
    }, {
      "id": 2,
      "usr": "c:@S@CompilationEntry@FI@args",
      "short_name": "args",
      "detailed_name": "std::vector<std::string> CompilationEntry::args",
      "definition_spelling": "9:28-9:32",
      "definition_extent": "9:3-9:32",
      "variable_type": 2,
      "declaring_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["9:28-9:32"]
    }]
}
*/