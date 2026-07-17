// base_string.h - length-prefixed strings. Not null-terminated by contract;
// use str8_to_cstr at the boundary with APIs that demand a C string.
//
// Include base.h, not this file directly. See STYLE.md for conventions.

#ifndef BASE_STRING_H
#define BASE_STRING_H

#include "base_types.h"

#ifndef BASE_ARENA_FWD
#define BASE_ARENA_FWD
typedef struct Arena Arena;
#endif

typedef struct Str8 Str8;
struct Str8
{
    u8 *str;
    u64 size;
};

// Wraps a string literal with no copy and no strlen. Only valid on literals
// and char arrays. On a char * it silently measures the pointer instead.
#define Str8Lit(s)  str8((u8 *)(s), sizeof(s) - 1)

// Expands to the two arguments printf's "%.*s" wants:
//     printf("%.*s\n", Str8VArg(name));
#define Str8VArg(s) (int)((s).size), ((s).str)

Str8 str8(u8 *str, u64 size);
Str8 str8_from_cstr(char *c);
Str8 str8_copy(Arena *arena, Str8 s);
Str8 str8_concat(Arena *arena, Str8 a, Str8 b);
char *str8_to_cstr(Arena *arena, Str8 s);
b32 str8_equal(Str8 a, Str8 b);
i32 str8_compare(Str8 a, Str8 b);

#ifdef BASE_IMPLEMENTATION

// Included here, not at the top. base_os.h needs the Str8 declarations above,
// so pulling the arena in earlier would close the cycle.
#include "base_arena.h"

Str8 str8(u8 *str, u64 size)
{
    Str8 result;
    result.str  = str;
    result.size = size;
    return result;
}

Str8 str8_from_cstr(char *c)
{
    u64 size = 0;
    if (c)
    {
        while (c[size] != 0)
        {
            size += 1;
        }
    }
    return str8((u8 *)c, size);
}

Str8 str8_copy(Arena *arena, Str8 s)
{
    Str8 result;
    result.str  = (u8 *)arena_push_no_zero(arena, s.size + 1, 1);
    result.size = s.size;
    if (result.str == 0)
    {
        result.size = 0;
        return result;
    }
    MemoryCopy(result.str, s.str, s.size);
    result.str[s.size] = 0; // not counted in size
    return result;
}

Str8 str8_concat(Arena *arena, Str8 a, Str8 b)
{
    Str8 result;
    result.str  = (u8 *)arena_push_no_zero(arena, a.size + b.size + 1, 1);
    result.size = a.size + b.size;
    if (result.str == 0)
    {
        result.size = 0;
        return result;
    }
    MemoryCopy(result.str, a.str, a.size);
    MemoryCopy(result.str + a.size, b.str, b.size);
    result.str[result.size] = 0;
    return result;
}

char *str8_to_cstr(Arena *arena, Str8 s)
{
    return (char *)str8_copy(arena, s).str;
}

b32 str8_equal(Str8 a, Str8 b)
{
    if (a.size != b.size)
    {
        return 0;
    }
    if (a.size == 0)
    {
        return 1;
    }
    return MemoryCompare(a.str, b.str, a.size) == 0;
}

// Lexicographic ordering: <0 if a sorts before b, 0 if equal, >0 otherwise.
i32 str8_compare(Str8 a, Str8 b)
{
    u64 common = Min(a.size, b.size);
    if (common > 0)
    {
        i32 order = (i32)MemoryCompare(a.str, b.str, common);
        if (order != 0)
        {
            return order;
        }
    }
    if (a.size == b.size)
    {
        return 0;
    }
    return (a.size < b.size) ? -1 : 1;
}

#endif // BASE_IMPLEMENTATION
#endif // BASE_STRING_H
