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
  "types": [{
      "id": 0,
      "usr": "c:@S@CompilationEntry",
      "short_name": "CompilationEntry",
      "qualified_name": "CompilationEntry",
      "definition_spelling": "6:8-6:24",
      "definition_extent": "6:1-10:2",
      "vars": [0, 1, 2],
      "uses": ["*6:8-6:24", "*12:13-12:29"]
    }, {
      "id": 1,
      "usr": "c:@N@std@T@string",
      "instantiations": [0, 1],
      "uses": ["*7:8-7:14", "*8:8-8:14", "*9:20-9:26"]
    }, {
      "id": 2,
      "usr": "c:@N@std@ST>2#T#T@vector",
      "instantiations": [2],
      "uses": ["*9:8-9:14", "*12:6-12:12"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@LoadCompilationEntriesFromDirectory#&1$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#",
      "short_name": "LoadCompilationEntriesFromDirectory",
      "qualified_name": "LoadCompilationEntriesFromDirectory",
      "declarations": ["12:31-12:66"],
      "uses": ["12:31-12:66"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@CompilationEntry@FI@directory",
      "short_name": "directory",
      "qualified_name": "CompilationEntry::directory",
      "definition_spelling": "7:15-7:24",
      "definition_extent": "7:3-7:24",
      "variable_type": 1,
      "declaring_type": 0,
      "uses": ["7:15-7:24"]
    }, {
      "id": 1,
      "usr": "c:@S@CompilationEntry@FI@filename",
      "short_name": "filename",
      "qualified_name": "CompilationEntry::filename",
      "definition_spelling": "8:15-8:23",
      "definition_extent": "8:3-8:23",
      "variable_type": 1,
      "declaring_type": 0,
      "uses": ["8:15-8:23"]
    }, {
      "id": 2,
      "usr": "c:@S@CompilationEntry@FI@args",
      "short_name": "args",
      "qualified_name": "CompilationEntry::args",
      "definition_spelling": "9:28-9:32",
      "definition_extent": "9:3-9:32",
      "variable_type": 2,
      "declaring_type": 0,
      "uses": ["9:28-9:32"]
    }]
}
*/