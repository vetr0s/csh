// repl.h - the read, compile, run, print loop.
//
// The module owns the session arena and the executable reservation. Nothing
// survives a line yet: repl_eval_ pops the arena back after every expression.

#ifndef REPL_H
#define REPL_H

#include "../base/base.h"
#include "../jit/jit.h"

#define REPL_LINE_MAX 4096

typedef struct Repl Repl;
struct Repl
{
    Arena *arena;
    Jit jit;
};

b32 repl_init(Repl *repl);
void repl_run(Repl *repl);
void repl_shutdown(Repl *repl);

#endif // REPL_H
