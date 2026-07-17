// sym.h - the session's symbol table.
//
// A symbol's storage is a slot in the session arena, so its address is fixed
// the moment it is declared. That is what lets a line compiled later reach a
// variable declared earlier: codegen bakes the slot's address straight into the
// instruction stream. vsh owns the storage, not the compiler.
//
// Declaring a name that already exists makes a new slot and shadows the old
// one. The old slot stays alive, because code compiled earlier holds its
// address and must keep reading what it was compiled against. That is lexical
// scoping, and it is also the only thing that can work once a name can be
// redeclared at a different type.
//
// The table only ever hands out values, never a Symbol *. Symbol records move
// when the array grows, so a pointer into it would dangle.

#ifndef SYM_H
#define SYM_H

#include "../base/base.h"

typedef struct Symbol Symbol;
struct Symbol
{
    Str8 name;
    i64 *slot;
    b32 is_const;
};

ArrayDef(SymbolArray, Symbol);

typedef struct SymTable SymTable;
struct SymTable
{
    SymbolArray symbols;
};

// A copy of what the table knows about one name. `slot` is 0 when the name is
// not declared.
typedef struct SymRef SymRef;
struct SymRef
{
    i64 *slot;
    b32 is_const;
};

// `arena` is the session arena. It must outlive every compiled line, because
// compiled code holds raw addresses into it.
void sym_init(SymTable *table, Arena *arena);

// The newest declaration of `name`.
SymRef sym_lookup(SymTable *table, Str8 name);

// Always a new slot, zeroed. Any earlier declaration of the same name is
// shadowed rather than replaced.
SymRef sym_declare(SymTable *table, Str8 name, b32 is_const);

#endif // SYM_H
