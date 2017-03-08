enum VarType {};

struct Holder {
  static constexpr VarType static_var = (VarType)0x0;
};

const VarType Holder::static_var;


/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@E@VarType",
      "short_name": "VarType",
      "qualified_name": "VarType",
      "definition": "1:1:6",
      "uses": ["*1:1:6", "*1:4:20", "*1:4:42", "*1:7:7"]
    }, {
      "id": 1,
      "usr": "c:@S@Holder",
      "short_name": "Holder",
      "qualified_name": "Holder",
      "definition": "1:3:8",
      "vars": [0],
      "uses": ["*1:3:8", "1:7:15"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Holder@static_var",
      "short_name": "static_var",
      "qualified_name": "Holder::static_var",
      "declaration": "1:4:28",
      "definition": "1:7:23",
      "variable_type": 0,
      "declaring_type": 1,
      "uses": ["1:4:28", "1:7:23"]
    }]
}
*/

























//#include <string>
//#include <xiosbase>

//#include <sstream>
//#include <algorithm>
//#include <vector>
//#include <string>
//#include <cstddef>
//#include <sstream>
//#include <iomanip>
//#include <limits>
//#include <vector>
//#include <cstddef>
//#include <tuple>
//#include <type_traits>
//#include <string>
//#include <string>
//#include <type_traits>
//#include <iterator>
//#include <vector>
//#include <string>
//#include <stdlib.h>
//#include <string>
//#include <vector>
//#include <string>
//#include <cstddef>
//#include <cmath>
//#include <limits>
//#include <type_traits>
//#include <set>
//#include <string>
//#include <vector>
//#include <iosfwd>
//#include <streambuf>
//#include <ostream>
//#include <fstream>
//#include <memory>
//#include <vector>
//#include <string>
//#include <stdexcept>
//#include <string>
//#include <vector>
//#include <sstream>
//#include <algorithm>
