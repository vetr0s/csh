// sym.h - the session's symbol table.
//
// A symbol's storage is a slot in the session arena, so its address is fixed
// the moment it is declared. That is what lets a line compiled later reach a
// variable declared earlier: codegen bakes the slot's address straight into the
// instruction stream. csh owns the storage, not the compiler.
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

// 0 when `name` is not declared.
i64 *sym_lookup(SymTable *table, Str8 name);

// Redeclaring a name hands back the existing slot rather than erroring, so code
// already compiled against that address keeps working.
i64 *sym_declare(SymTable *table, Str8 name);

#endif // SYM_H
