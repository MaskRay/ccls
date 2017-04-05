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
      "definition": "6:8",
      "vars": [0, 1, 2],
      "uses": ["*6:8", "*12:13"]
    }, {
      "id": 1,
      "usr": "c:@N@std@T@string",
      "instantiations": [0, 1],
      "uses": ["*7:8", "*8:8", "*9:20"]
    }, {
      "id": 2,
      "usr": "c:@N@std@ST>2#T#T@vector",
      "instantiations": [2],
      "uses": ["*9:8", "*12:6"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@LoadCompilationEntriesFromDirectory#&1$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#",
      "short_name": "LoadCompilationEntriesFromDirectory",
      "qualified_name": "LoadCompilationEntriesFromDirectory",
      "declarations": ["12:31"],
      "uses": ["12:31"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@CompilationEntry@FI@directory",
      "short_name": "directory",
      "qualified_name": "CompilationEntry::directory",
      "definition": "7:15",
      "variable_type": 1,
      "declaring_type": 0,
      "uses": ["7:15"]
    }, {
      "id": 1,
      "usr": "c:@S@CompilationEntry@FI@filename",
      "short_name": "filename",
      "qualified_name": "CompilationEntry::filename",
      "definition": "8:15",
      "variable_type": 1,
      "declaring_type": 0,
      "uses": ["8:15"]
    }, {
      "id": 2,
      "usr": "c:@S@CompilationEntry@FI@args",
      "short_name": "args",
      "qualified_name": "CompilationEntry::args",
      "definition": "9:28",
      "variable_type": 2,
      "declaring_type": 0,
      "uses": ["9:28"]
    }]
}
*/