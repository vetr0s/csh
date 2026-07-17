// base_types.h - fixed-width types and the macros every other file depends on.
//
// Include base.h, not this file directly. See STYLE.md for conventions.

#ifndef BASE_TYPES_H
#define BASE_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

////////////////////////////////
// Build configuration
//
// The build scripts pass -DBUILD_DEBUG=1 or -DBUILD_DEBUG=0. Default to a
// debug build so a bare `clang src/main.c` still behaves sensibly.

#if !defined(BUILD_DEBUG)
#define BUILD_DEBUG 1
#endif

////////////////////////////////
// Fixed-width types
//
// These are the only scalar types used past this file. No int/long/unsigned.

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef i32 b32;

////////////////////////////////
// Basic macros

#define ArrayCount(a)    (sizeof(a) / sizeof((a)[0]))
#define Min(a, b)        ((a) < (b) ? (a) : (b))
#define Max(a, b)        ((a) > (b) ? (a) : (b))
#define Clamp(x, lo, hi) Min(Max((x), (lo)), (hi))
#define Unused(x)        ((void)(x))

// `b` must be a power of two. `T` must be a type name, not an expression.
// C11 _Alignof and MSVC __alignof both reject expressions.
#define AlignPow2(x, b)  (((x) + ((b) - 1)) & (~((b) - 1)))
#if defined(_MSC_VER)
#define AlignOf(T) __alignof(T)
#else
#define AlignOf(T) _Alignof(T)
#endif

#define Kilobytes(n)            ((u64)(n) << 10)
#define Megabytes(n)            ((u64)(n) << 20)
#define Gigabytes(n)            ((u64)(n) << 30)

#define Glue_(a, b)             a##b
#define Glue(a, b)              Glue_(a, b)

#define MemoryZero(p, size)     memset((p), 0, (size))
#define MemoryZeroStruct(p)     MemoryZero((p), sizeof(*(p)))
#define MemoryCopy(dst, src, n) memmove((dst), (src), (n))
#define MemoryCompare(a, b, n)  memcmp((a), (b), (n))

////////////////////////////////
// Assertions
//
// Trap() breaks into an attached debugger rather than calling abort(), so the
// failing frame is still live when execution stops.

#if defined(_MSC_VER)
#define Trap() __debugbreak()
#elif defined(__clang__)
#define Trap() __builtin_debugtrap()
#elif defined(__GNUC__)
#define Trap() __builtin_trap()
#else
#define Trap() (*(volatile int *)0 = 0)
#endif

#if BUILD_DEBUG
#define Assert(x)                                                                                  \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            Trap();                                                                                \
        }                                                                                          \
    } while (0)
#else
#define Assert(x) ((void)0)
#endif

// Compiles to nothing on success, to a negative-sized array on failure.
#define StaticAssert(c, id) typedef char Glue(static_assert_##id##_, __LINE__)[(c) ? 1 : -1]

#endif // BASE_TYPES_H
