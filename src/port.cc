#include "port.h"

#include <stdio.h>
#include <stdlib.h>

void cquery_unreachable_internal(const char* msg, const char* file, int line) {
  fprintf(stderr, "unreachable %s:%d %s\n", file, line, msg);
  CQUERY_BUILTIN_UNREACHABLE;
  abort();
}
