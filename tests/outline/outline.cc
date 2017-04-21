#include <vector>

struct MergeableUpdate {
  int a;
  int b;
  std::vector<int> to_add;
};

/*
OUTPUT:
{
  "dependencies": ["C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vadefs.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/yvals.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/stdint.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstdint", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_malloc.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/stddef.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_search.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wstdlib.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/stdlib.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstdlib", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/ymath.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/float.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/math.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cmath", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xtr1common", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xtgmath.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/errno.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime_string.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_memcpy_s.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_stdio_config.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wconio.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wctype.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wdirect.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wio.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wprocess.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wstdio.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wstring.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_wtime.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/sys/types.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/sys/stat.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/wchar.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cwchar", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstddef", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/initializer_list", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xstddef", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/limits", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/type_traits", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/exception", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/malloc.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_terminate.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/eh.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime_exception.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime_new.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/new", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/stdio.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstdio", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/corecrt_memory.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/string.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/cstring", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vcruntime_new_debug.h", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.14393.0/ucrt/crtdbg.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/iosfwd", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/utility", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xutility", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xmemory0", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xatomic0.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/setjmp.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/mmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/emmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/pmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/tmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/smmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/nmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/wmmintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/immintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/ammintrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/intrin.h", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xmemory", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/xstring", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/stdexcept", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/vector"],
  "types": [{
      "id": 0,
      "usr": "c:@S@MergeableUpdate",
      "short_name": "MergeableUpdate",
      "detailed_name": "MergeableUpdate",
      "definition_spelling": "3:8-3:23",
      "definition_extent": "3:1-7:2",
      "vars": [0, 1, 2],
      "uses": ["3:8-3:23"]
    }, {
      "id": 1,
      "usr": "c:@N@std@ST>2#T#T@vector",
      "instances": [2],
      "uses": ["6:8-6:14"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@MergeableUpdate@FI@a",
      "short_name": "a",
      "detailed_name": "int MergeableUpdate::a",
      "definition_spelling": "4:7-4:8",
      "definition_extent": "4:3-4:8",
      "declaring_type": 0,
      "uses": ["4:7-4:8"]
    }, {
      "id": 1,
      "usr": "c:@S@MergeableUpdate@FI@b",
      "short_name": "b",
      "detailed_name": "int MergeableUpdate::b",
      "definition_spelling": "5:7-5:8",
      "definition_extent": "5:3-5:8",
      "declaring_type": 0,
      "uses": ["5:7-5:8"]
    }, {
      "id": 2,
      "usr": "c:@S@MergeableUpdate@FI@to_add",
      "short_name": "to_add",
      "detailed_name": "std::vector<int> MergeableUpdate::to_add",
      "definition_spelling": "6:20-6:26",
      "definition_extent": "6:3-6:26",
      "variable_type": 1,
      "declaring_type": 0,
      "uses": ["6:20-6:26"]
    }]
}
*/
