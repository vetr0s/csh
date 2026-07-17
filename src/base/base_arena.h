// base_arena.h - linear allocator over a reserved virtual address range.
//
// An arena reserves a large range of address space up front and commits pages
// only as the bump pointer passes them. Reserving a gigabyte costs nothing
// until it is touched, and pointers into the arena stay valid forever. There is
// no per-allocation free. Memory comes back by popping to a previous position
// or by resetting the whole arena.
//
// The bookkeeping lives in the first ARENA_HEADER_SIZE bytes of the arena's own
// reserved range, so `Arena *` is the base of that range.
//
// Include base.h, not this file directly. See STYLE.md for conventions.

#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#include "base_types.h"

#ifndef BASE_ARENA_FWD
#define BASE_ARENA_FWD
typedef struct Arena Arena;
#endif

#define ARENA_HEADER_SIZE          64            // keeps the first push cache-aligned
#define ARENA_COMMIT_GRANULARITY   Kilobytes(64) // multiple of every target's page size
#define ARENA_DEFAULT_RESERVE_SIZE Gigabytes(1)

struct Arena
{
    u64 reserved;  // bytes of address space reserved, including the header
    u64 committed; // bytes backed by physical pages, including the header
    u64 pos;       // bump pointer, as an offset from the base of the arena
};

typedef struct Temp Temp;
struct Temp
{
    Arena *arena;
    u64 pos;
};

Arena *arena_alloc(u64 reserve_size);
void arena_release(Arena *arena);

// arena_push zeroes what it returns. arena_push_no_zero does not, and only pays
// off when the caller overwrites the whole block.
void *arena_push(Arena *arena, u64 size, u64 align);
void *arena_push_no_zero(Arena *arena, u64 size, u64 align);

u64 arena_pos(Arena *arena);
void arena_pop(Arena *arena, u64 amount);
void arena_pop_to(Arena *arena, u64 pos);
void arena_reset(Arena *arena);

#define PushStruct(arena, T)       ((T *)arena_push((arena), sizeof(T), AlignOf(T)))
#define PushArray(arena, T, count) ((T *)arena_push((arena), sizeof(T) * (u64)(count), AlignOf(T)))
#define PushArrayNoZero(arena, T, count)                                                           \
    ((T *)arena_push_no_zero((arena), sizeof(T) * (u64)(count), AlignOf(T)))

// Scratch scope: everything pushed between begin and end is released at end.
//
//     Temp scratch = temp_begin(arena);
//     ... push freely ...
//     temp_end(scratch);
Temp temp_begin(Arena *arena);
void temp_end(Temp temp);

#ifdef BASE_IMPLEMENTATION

// Included here, not at the top, so arena_push is declared before base_os.h's
// implementation expands. That implementation allocates from an arena.
#include "base_os.h"

Arena *arena_alloc(u64 reserve_size)
{
    u64 page_size = os_page_size();
    reserve_size  = AlignPow2(reserve_size, page_size);
    if (reserve_size < ARENA_HEADER_SIZE)
    {
        return 0;
    }

    void *base = os_reserve(reserve_size);
    if (base == 0)
    {
        return 0;
    }

    u64 initial_commit = Min(AlignPow2((u64)ARENA_HEADER_SIZE, page_size), reserve_size);
    if (!os_commit(base, initial_commit))
    {
        os_release(base, reserve_size);
        return 0;
    }

    Arena *arena     = (Arena *)base;
    arena->reserved  = reserve_size;
    arena->committed = initial_commit;
    arena->pos       = ARENA_HEADER_SIZE;
    return arena;
}

void arena_release(Arena *arena)
{
    if (arena)
    {
        os_release(arena, arena->reserved);
    }
}

void *arena_push_no_zero(Arena *arena, u64 size, u64 align)
{
    Assert(arena != 0);
    Assert(align > 0 && (align & (align - 1)) == 0); // power of two

    u64 pos_aligned = AlignPow2(arena->pos, align);
    u64 new_pos     = pos_aligned + size;

    if (new_pos > arena->reserved)
    {
        Assert(!"arena exhausted: reserve more address space in arena_alloc");
        return 0;
    }

    if (new_pos > arena->committed)
    {
        u64 commit_target = AlignPow2(new_pos, ARENA_COMMIT_GRANULARITY);
        commit_target     = Min(commit_target, arena->reserved);
        u8 *commit_base   = (u8 *)arena + arena->committed;
        if (!os_commit(commit_base, commit_target - arena->committed))
        {
            return 0;
        }
        arena->committed = commit_target;
    }

    arena->pos = new_pos;
    return (u8 *)arena + pos_aligned;
}

void *arena_push(Arena *arena, u64 size, u64 align)
{
    void *result = arena_push_no_zero(arena, size, align);
    if (result != 0 && size > 0)
    {
        MemoryZero(result, size);
    }
    return result;
}

u64 arena_pos(Arena *arena)
{
    Assert(arena != 0);
    return arena->pos;
}

void arena_pop_to(Arena *arena, u64 pos)
{
    Assert(arena != 0);
    // Pages stay committed on pop. Returning them would only fault them back in.
    arena->pos = Max((u64)ARENA_HEADER_SIZE, pos);
}

void arena_pop(Arena *arena, u64 amount)
{
    Assert(arena != 0);
    u64 pos = arena->pos;
    arena_pop_to(arena, (amount < pos) ? (pos - amount) : 0);
}

void arena_reset(Arena *arena)
{
    arena_pop_to(arena, ARENA_HEADER_SIZE);
}

Temp temp_begin(Arena *arena)
{
    Temp temp;
    temp.arena = arena;
    temp.pos   = arena_pos(arena);
    return temp;
}

void temp_end(Temp temp)
{
    arena_pop_to(temp.arena, temp.pos);
}

#endif // BASE_IMPLEMENTATION
#endif // BASE_ARENA_H
