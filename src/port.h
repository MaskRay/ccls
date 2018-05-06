#pragma once

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if defined(__GNUC__)
#define ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define ATTRIBUTE_UNUSED
#endif

#ifdef __clang__
#define GUARDED_BY(x)  __attribute__((guarded_by(x)))
#endif

// TODO GCC
#if __has_builtin(__builtin_unreachable)
#define CCLS_BUILTIN_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#define CCLS_BUILTIN_UNREACHABLE __assume(false)
#else
#define CCLS_BUILTIN_UNREACHABLE
#endif

void ccls_unreachable_internal(const char* msg, const char* file, int line);
#ifndef NDEBUG
#define CCLS_UNREACHABLE(msg) \
  ccls_unreachable_internal(msg, __FILE__, __LINE__)
#else
#define CCLS_UNREACHABLE(msg)
#endif
