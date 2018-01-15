#pragma once

#if defined(__GNUC__) && !defined(__clang__)
#define ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define ATTRIBUTE_UNUSED
#endif
