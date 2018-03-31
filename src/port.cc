#include "port.h"

#include <stdio.h>
#include <stdlib.h>

void ccls_unreachable_internal(const char* msg, const char* file, int line) {
  fprintf(stderr, "unreachable %s:%d %s\n", file, line, msg);
  CCLS_BUILTIN_UNREACHABLE;
  abort();
}
