// base_array.h - typed dynamic arrays, arena-backed.
//
// Declare a type once, then use the macros on a pointer to it:
//
//     ArrayDef(U64Array, u64);
//
//     U64Array numbers;
//     ArrayInit(&numbers, arena);
//     ArrayReserve(&numbers, 64);
//     ArrayPush(&numbers, 7);
//
// Growth is not free here. An arena cannot realloc, so a grow that cannot be
// satisfied in place copies to a new block and abandons the old one for the
// life of the arena. The array does extend in place when its block is the
// arena's most recent allocation. Reserving the final count up front avoids
// both cases.
//
// Include base.h, not this file directly. See STYLE.md for conventions.

#ifndef BASE_ARRAY_H
#define BASE_ARRAY_H

#include "base_arena.h"
#include "base_types.h"

// C11 _Alignof and MSVC __alignof both require a type name, and these macros
// only ever hold an expression. Blocks take the strictest alignment instead.
#define ARRAY_ALIGN         16
#define ARRAY_INITIAL_COUNT 8

#define ArrayDef(Name, T)                                                                          \
    typedef struct Name Name;                                                                      \
    struct Name                                                                                    \
    {                                                                                              \
        Arena *arena;                                                                              \
        T *v;                                                                                      \
        u64 count;                                                                                 \
        u64 capacity;                                                                              \
    }

#define ArrayInit(a, arena_ptr)                                                                    \
    do                                                                                             \
    {                                                                                              \
        (a)->arena    = (arena_ptr);                                                               \
        (a)->v        = 0;                                                                         \
        (a)->count    = 0;                                                                         \
        (a)->capacity = 0;                                                                         \
    } while (0)

// Ensures room for at least `n` elements. Evaluates to the element pointer,
// which moves on growth and goes null if the arena is exhausted.
#define ArrayReserve(a, n)                                                                         \
    ((a)->v =                                                                                      \
         array_grow_((a)->arena, (a)->v, (a)->count, &(a)->capacity, sizeof(*(a)->v), (u64)(n)))

#define ArrayPush(a, item)                                                                         \
    do                                                                                             \
    {                                                                                              \
        ArrayReserve((a), (a)->count + 1);                                                         \
        Assert((a)->v != 0);                                                                       \
        (a)->v[(a)->count] = (item);                                                               \
        (a)->count += 1;                                                                           \
    } while (0)

#define ArrayPop(a)                                                                                \
    do                                                                                             \
    {                                                                                              \
        Assert((a)->count > 0);                                                                    \
        (a)->count -= 1;                                                                           \
    } while (0)

#define ArrayLast(a)  ((a)->v[(a)->count - 1])
#define ArrayClear(a) ((a)->count = 0)

// Internal. Call through the macros above.
void *array_grow_(Arena *arena, void *old_v, u64 count, u64 *capacity, u64 item_size, u64 needed);

#ifdef BASE_IMPLEMENTATION

// A failed grow returns null and zeroes the capacity. Handing back the old
// short block would let the next push run off the end of it.
void *array_grow_(Arena *arena, void *old_v, u64 count, u64 *capacity, u64 item_size, u64 needed)
{
    u64 capacity_old = *capacity;
    if (needed <= capacity_old)
    {
        return old_v;
    }

    u64 capacity_new = (capacity_old == 0) ? ARRAY_INITIAL_COUNT : capacity_old;
    while (capacity_new < needed)
    {
        capacity_new *= 2;
    }
    u64 bytes_added = (capacity_new - capacity_old) * item_size;

    if (old_v != 0)
    {
        // Offsetting a null pointer is undefined even by zero, so this stays
        // inside the null check.
        u8 *block_end = (u8 *)old_v + capacity_old * item_size;

        // Fast path: the block is the arena's last allocation, so extend it.
        if (block_end == (u8 *)arena + arena_pos(arena))
        {
            if (arena_push_no_zero(arena, bytes_added, 1) == 0)
            {
                *capacity = 0;
                return 0;
            }
            MemoryZero(block_end, bytes_added);
            *capacity = capacity_new;
            return old_v;
        }
    }

    void *v_new = arena_push(arena, capacity_new * item_size, ARRAY_ALIGN);
    if (v_new == 0)
    {
        *capacity = 0;
        return 0;
    }
    if (old_v != 0 && count > 0)
    {
        MemoryCopy(v_new, old_v, count * item_size);
    }
    *capacity = capacity_new;
    return v_new;
}

#endif // BASE_IMPLEMENTATION
#endif // BASE_ARRAY_H
