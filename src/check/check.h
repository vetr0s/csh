// check.h - name resolution, between parse and codegen.
//
// Fills in each node's slot so codegen never has to ask what a name means. When
// vsh grows a second type, this is where the type checker goes.

#ifndef CHECK_H
#define CHECK_H

#include "../base/base.h"
#include "../parse/parse.h"
#include "../sym/sym.h"

// Declares the name of a NodeKind_Decl and resolves every NodeKind_Var against
// `syms`. Names outlive the line, so they are declared into the session arena
// that `syms` was built on. `arena` only carries the error message back.
b32 check_resolve(Arena *arena, SymTable *syms, Node *ast, Str8 *out_err);

#endif // CHECK_H
