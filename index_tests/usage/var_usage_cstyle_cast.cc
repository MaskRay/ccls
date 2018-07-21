enum VarType {};

struct Holder {
  static constexpr VarType static_var = (VarType)0x0;
};

const VarType Holder::static_var;


/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 5792006888140599735,
      "detailed_name": "enum VarType {\n}",
      "qual_name_offset": 5,
      "short_name": "VarType",
      "kind": 10,
      "declarations": [],
      "spell": "1:6-1:13|0|1|2",
      "extent": "1:1-1:16|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [7057400933868440116, 7057400933868440116],
      "uses": ["4:20-4:27|10028537921178202800|2|4", "4:42-4:49|10028537921178202800|2|4", "7:7-7:14|0|1|4"]
    }, {
      "usr": 10028537921178202800,
      "detailed_name": "struct Holder {}",
      "qual_name_offset": 7,
      "short_name": "Holder",
      "kind": 23,
      "declarations": [],
      "spell": "3:8-3:14|0|1|2",
      "extent": "3:1-5:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:15-7:21|0|1|4"]
    }],
  "usr2var": [{
      "usr": 7057400933868440116,
      "detailed_name": "static constexpr VarType Holder::static_var",
      "qual_name_offset": 25,
      "short_name": "static_var",
      "hover": "static constexpr VarType Holder::static_var = (VarType)0x0",
      "declarations": ["4:28-4:38|10028537921178202800|2|1025"],
      "spell": "7:23-7:33|10028537921178202800|2|1026",
      "extent": "7:1-7:33|10028537921178202800|2|0",
      "type": 5792006888140599735,
      "uses": [],
      "kind": 13,
      "storage": 2
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
