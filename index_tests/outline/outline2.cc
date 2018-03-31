//#pragma once

#include <string>
#include <vector>

struct CompilationEntry {
  std::string directory;
  std::string filename;
  std::vector<std::string> args;
};

std::vector<CompilationEntry> LoadCompilationEntriesFromDirectory(const std::string& project_directory);

/*
TEXT_REPLACE:
std::__1::vector <===> std::vector
std::__1::string <===> std::string
std::__cxx11::string <===> std::string
c:@N@std@string <===> c:@N@std@N@__1@T@string
c:@N@std@T@string <===> c:@N@std@N@__1@T@string
c:@N@std@N@__cxx11@T@string <===> c:@N@std@N@__1@T@string
c:@N@std@ST>2#T#T@vector <===> c:@N@std@N@__1@ST>2#T#T@vector
c:@F@LoadCompilationEntriesFromDirectory#&1$@N@std@N@__1@S@basic_string>#C#$@N@std@N@__1@S@char_traits>#C#$@N@std@N@__1@S@allocator>#C# <===> c:@F@LoadCompilationEntriesFromDirectory#&1$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#
c:@F@LoadCompilationEntriesFromDirectory#&1$@N@std@N@__cxx11@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C# <===> c:@F@LoadCompilationEntriesFromDirectory#&1$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#
4160338041907786 <===> 14151982074805896770
7543170857910783654 <===> 14151982074805896770
9802818309312685221 <===> 11244864715202245734
7636646237071509980 <===> 14151982074805896770
9178760565669096175 <===> 10956461108384510180
10468929532989002392 <===> 11244864715202245734
4160338041907786 <===> 14151982074805896770
9802818309312685221 <===> 11244864715202245734

OUTPUT:
{
  "includes": [{
      "line": 2,
      "resolved_path": "&string"
    }, {
      "line": 3,
      "resolved_path": "&vector"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 4992269036372211530,
      "detailed_name": "CompilationEntry",
      "short_name": "CompilationEntry",
      "kind": 23,
      "declarations": [],
      "spell": "6:8-6:24|-1|1|2",
      "extent": "6:1-10:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1, 2],
      "instances": [],
      "uses": ["12:13-12:29|-1|1|4"]
    }, {
      "id": 1,
      "usr": 14151982074805896770,
      "detailed_name": "std::string",
      "short_name": "string",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": ["7:8-7:14|-1|1|4", "8:8-8:14|-1|1|4", "9:20-9:26|-1|1|4", "12:78-12:84|-1|1|4"]
    }, {
      "id": 2,
      "usr": 5401847601697785946,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:3-7:6|0|2|4", "8:3-8:6|0|2|4", "9:3-9:6|0|2|4", "9:15-9:18|0|2|4", "12:1-12:4|-1|1|4", "12:73-12:76|-1|1|4"]
    }, {
      "id": 3,
      "usr": 10956461108384510180,
      "detailed_name": "std::vector",
      "short_name": "vector",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2],
      "uses": ["9:8-9:14|-1|1|4", "12:6-12:12|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 11244864715202245734,
      "detailed_name": "std::vector<CompilationEntry> LoadCompilationEntriesFromDirectory(const std::string &project_directory)",
      "short_name": "LoadCompilationEntriesFromDirectory",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "12:31-12:66|-1|1|1",
          "param_spellings": ["12:86-12:103"]
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 1153224798516629792,
      "detailed_name": "std::string CompilationEntry::directory",
      "short_name": "directory",
      "declarations": [],
      "spell": "7:15-7:24|0|2|2",
      "extent": "7:3-7:24|0|2|0",
      "type": 1,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "id": 1,
      "usr": 2255668374222866345,
      "detailed_name": "std::string CompilationEntry::filename",
      "short_name": "filename",
      "declarations": [],
      "spell": "8:15-8:23|0|2|2",
      "extent": "8:3-8:23|0|2|0",
      "type": 1,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "id": 2,
      "usr": 12616880765274259414,
      "detailed_name": "std::vector<std::string> CompilationEntry::args",
      "short_name": "args",
      "declarations": [],
      "spell": "9:28-9:32|0|2|2",
      "extent": "9:3-9:32|0|2|0",
      "type": 3,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
