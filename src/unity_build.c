// unity_build.c - the whole program, assembled.
//
// Defining BASE_IMPLEMENTATION before base.h pulls in the base layer's
// definitions as well as its declarations; this is the one place in the program
// that does so.
//
// Every new project .c file gets an #include here, in dependency order, and is
// never passed to the compiler itself. Only src/main.c is compiled.

#define BASE_IMPLEMENTATION
#include "base/base.h"

#include "lex/lex.c"
#include "parse/parse.c"
#include "jit/jit.c"
#include "repl/repl.c"
