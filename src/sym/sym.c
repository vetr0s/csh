// sym.c - see sym.h.

#include "sym.h"

void sym_init(SymTable *table, Arena *arena)
{
    MemoryZeroStruct(table);
    ArrayInit(&table->symbols, arena);
}

// Backward, so the newest declaration of a name wins. Shadowed entries stay in
// the table because code compiled against their slots is still out there.
//
// Linear scan. A session holds few enough names that a hash map would cost more
// code than it saves.
SymRef sym_lookup(SymTable *table, Str8 name)
{
    SymRef ref = {0};
    for (u64 i = table->symbols.count; i > 0; i -= 1)
    {
        Symbol *symbol = &table->symbols.v[i - 1];
        if (str8_equal(symbol->name, name))
        {
            ref.slot     = symbol->slot;
            ref.is_const = symbol->is_const;
            break;
        }
    }
    return ref;
}

SymRef sym_declare(SymTable *table, Str8 name, b32 is_const)
{
    SymRef ref   = {0};
    Arena *arena = table->symbols.arena;

    i64 *slot = PushStruct(arena, i64);
    if (slot == 0)
    {
        return ref;
    }

    // The name is copied because it points into the line buffer, which the next
    // prompt overwrites.
    Symbol symbol   = {0};
    symbol.name     = str8_copy(arena, name);
    symbol.slot     = slot;
    symbol.is_const = is_const;
    if (symbol.name.str == 0)
    {
        return ref;
    }

    ArrayPush(&table->symbols, symbol);

    ref.slot     = slot;
    ref.is_const = is_const;
    return ref;
}
