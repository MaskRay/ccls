#pragma once

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if defined(__GNUC__)
#define ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define ATTRIBUTE_UNUSED
#endif

// TODO GCC
#if __has_builtin(__builtin_unreachable)
#define CQUERY_BUILTIN_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#define CQUERY_BUILTIN_UNREACHABLE __assume(false)
#else
#define CQUERY_BUILTIN_UNREACHABLE
#endif

void cquery_unreachable_internal(const char* msg, const char* file, int line);
#ifndef NDEBUG
#define CQUERY_UNREACHABLE(msg) \
  cquery_unreachable_internal(msg, __FILE__, __LINE__)
#else
#define CQUERY_UNREACHABLE(msg)
#endif
