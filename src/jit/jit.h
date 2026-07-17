// jit.h - expression tree to arm64 machine code, callable in this process.
//
// A Jit owns one executable reservation and bump-allocates compiled functions
// out of it, the same shape as an Arena but over os_reserve_exec. Compiled code
// lives until jit_shutdown; there is no per-function free.

#ifndef JIT_H
#define JIT_H

#include "../base/base.h"
#include "../parse/parse.h"

#define JIT_DEFAULT_RESERVE_SIZE Megabytes(64)
#define JIT_COMMIT_GRANULARITY   Kilobytes(64)

typedef i64 (*JitFunc)(void);

// A failure the generated code can report at runtime. On a trap the code stores
// the kind and carries on with a zero rather than returning early, because
// returning from the middle of an expression would leave the machine stack
// holding operands that nothing would pop. The caller discards the value.
typedef enum TrapKind TrapKind;
enum TrapKind
{
    TrapKind_None,
    TrapKind_DivideByZero,
    TrapKind_COUNT
};

char *jit_trap_message(TrapKind kind);

typedef struct Jit Jit;
struct Jit
{
    u8 *base;
    u64 reserved;
    u64 committed;
    u64 pos;
};

b32 jit_init(Jit *jit, u64 reserve_size);
void jit_shutdown(Jit *jit);

// Compiles `ast` into `jit` and hands back a callable through `out`. The
// instruction buffer is built on `scratch`, which is left where it was found.
//
// `trap` is where the generated code reports a runtime failure. Its address is
// baked into the code, so it must outlive every function compiled against it.
// Clear it before calling and read it after.
b32 jit_compile(Jit *jit, Arena *scratch, Node *ast, i64 *trap, JitFunc *out);

#endif // JIT_H
