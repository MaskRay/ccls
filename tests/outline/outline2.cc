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
      "resolved_path": "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/string"
    }, {
      "line": 4,
      "resolved_path": "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vector"
    }],
  "dependencies": ["C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/string", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/istream", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/ostream", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/ios", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xlocnum", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/climits", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/yvals.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/crtdefs.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/sal.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/limits.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cmath", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/math.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xtgmath.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xtr1common", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstdlib", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/stdlib.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_malloc.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_search.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/stddef.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wstdlib.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstdio", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/stdio.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wstdio.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_stdio_config.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/streambuf", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xiosbase", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xlocale", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstring", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/string.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_memory.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_memcpy_s.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/errno.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime_string.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wstring.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/stdexcept", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/exception", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/type_traits", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xstddef", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstddef", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/initializer_list", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/malloc.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime_exception.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/eh.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_terminate.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xstring", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xmemory0", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstdint", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/stdint.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/limits", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/ymath.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cfloat", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/float.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cwchar", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/wchar.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wconio.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wctype.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wdirect.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wio.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wprocess.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wtime.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/sys/stat.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/new", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime_new.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xutility", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/utility", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/iosfwd", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/crtdbg.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime_new_debug.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xatomic0.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/intrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/setjmp.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/immintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/wmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/nmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/smmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/tmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/pmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/emmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/typeinfo", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime_typeinfo.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xlocinfo", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xlocinfo.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/ctype.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/locale.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xfacet", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/system_error", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cerrno", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/share.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vector", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xmemory", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vadefs.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/sys/types.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/mmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/ammintrin.h"],
  "types": [{
      "id": 0,
      "usr": "c:@S@CompilationEntry",
      "short_name": "CompilationEntry",
      "detailed_name": "CompilationEntry",
      "definition_spelling": "6:8-6:24",
      "definition_extent": "6:1-10:2",
      "vars": [0, 1, 2],
      "uses": ["6:8-6:24", "12:13-12:29"]
    }, {
      "id": 1,
      "usr": "c:@N@std@T@string",
      "instances": [0, 1],
      "uses": ["7:8-7:14", "8:8-8:14", "9:20-9:26"]
    }, {
      "id": 2,
      "usr": "c:@N@std@ST>2#T#T@vector",
      "instances": [2],
      "uses": ["9:8-9:14", "12:6-12:12"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@LoadCompilationEntriesFromDirectory#&1$@N@std@S@basic_string>#C#$@N@std@S@char_traits>#C#$@N@std@S@allocator>#C#",
      "short_name": "LoadCompilationEntriesFromDirectory",
      "detailed_name": "std::vector<CompilationEntry> LoadCompilationEntriesFromDirectory(const std::string &)",
      "declarations": [{
          "spelling": "12:31-12:66",
          "extent": "12:1-12:104",
          "content": "std::vector<CompilationEntry> LoadCompilationEntriesFromDirectory(const std::string& project_directory)",
          "param_spellings": ["12:86-12:103"]
        }]
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
      "uses": ["9:28-9:32"]
    }]
}
*/