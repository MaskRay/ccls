#ifndef INDEX_H_
#define INDEX_H_
#include <clang-c/Index.h>

namespace clang {
  class Index {
  public:
    Index(int excludeDeclarationsFromPCH, int displayDiagnostics);
    ~Index();
    CXIndex cx_index;
  };
}  // namespace clang
#endif  // INDEX_H_
