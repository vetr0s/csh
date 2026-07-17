// repl.h - the read, compile, run, print loop.
//
// Two arenas, because a REPL has two lifetimes. Declarations live for the
// session and their slots must never move, since compiled code holds their
// addresses. Everything a single line needs to compile is garbage once it has
// run. The session arena is never popped; the scratch arena is popped every
// line.

#ifndef REPL_H
#define REPL_H

#include "../base/base.h"
#include "../jit/jit.h"
#include "../sym/sym.h"

#define REPL_LINE_MAX 4096

typedef struct Repl Repl;
struct Repl
{
    Arena *session; // symbols, their names, and their slots
    Arena *scratch; // tokens, tree, instruction buffer
    Jit jit;
    SymTable syms;
};

b32 repl_init(Repl *repl);
void repl_run(Repl *repl);
void repl_shutdown(Repl *repl);

#endif // REPL_H
