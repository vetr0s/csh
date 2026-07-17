// sym.c - see sym.h.

#include "sym.h"

void sym_init(SymTable *table, Arena *arena)
{
    MemoryZeroStruct(table);
    ArrayInit(&table->symbols, arena);
}

static Symbol *sym_find_(SymTable *table, Str8 name)
{
    // Linear scan. A session holds few enough names that a hash map would cost
    // more code than it saves.
    for (u64 i = 0; i < table->symbols.count; i += 1)
    {
        if (str8_equal(table->symbols.v[i].name, name))
        {
            return &table->symbols.v[i];
        }
    }
    return 0;
}

i64 *sym_lookup(SymTable *table, Str8 name)
{
    Symbol *symbol = sym_find_(table, name);
    return (symbol != 0) ? symbol->slot : 0;
}

i64 *sym_declare(SymTable *table, Str8 name)
{
    Symbol *existing = sym_find_(table, name);
    if (existing != 0)
    {
        return existing->slot;
    }

    Arena *arena = table->symbols.arena;

    i64 *slot = PushStruct(arena, i64);
    if (slot == 0)
    {
        return 0;
    }

    // The name is copied because it points into the line buffer, which the next
    // prompt overwrites.
    Symbol symbol = {0};
    symbol.name   = str8_copy(arena, name);
    symbol.slot   = slot;
    if (symbol.name.str == 0)
    {
        return 0;
    }

    ArrayPush(&table->symbols, symbol);
    return slot;
}
