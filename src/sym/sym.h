// sym.h - the session's symbol table.
//
// A symbol's storage is a slot in the session arena, so its address is fixed
// the moment it is declared. That is what lets a line compiled later reach a
// variable declared earlier: codegen bakes the slot's address straight into the
// instruction stream. csh owns the storage, not the compiler.
//
// Declaring a name that already exists makes a new slot and shadows the old
// one. The old slot stays alive, because code compiled earlier holds its
// address and must keep reading what it was compiled against. That is lexical
// scoping, and it is also the only thing that can work once a name can be
// redeclared at a different type.
//
// The table only ever hands out slot pointers. Symbol records move when the
// array grows, so nothing outside sym.c holds a Symbol *.

#ifndef SYM_H
#define SYM_H

#include "../base/base.h"

typedef struct Symbol Symbol;
struct Symbol
{
    Str8 name;
    i64 *slot;
};

ArrayDef(SymbolArray, Symbol);

typedef struct SymTable SymTable;
struct SymTable
{
    SymbolArray symbols;
};

// `arena` is the session arena. It must outlive every compiled line, because
// compiled code holds raw addresses into it.
void sym_init(SymTable *table, Arena *arena);

// The newest declaration of `name`, or 0 when there is none.
i64 *sym_lookup(SymTable *table, Str8 name);

// Always a new slot, zeroed. Any earlier declaration of the same name is
// shadowed rather than replaced.
i64 *sym_declare(SymTable *table, Str8 name);

#endif // SYM_H
