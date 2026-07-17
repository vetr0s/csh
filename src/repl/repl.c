// repl.c - see repl.h.

#include "repl.h"

#include "../parse/parse.h"

#include <stdio.h>

b32 repl_init(Repl *repl)
{
    MemoryZeroStruct(repl);

    repl->arena = arena_alloc(ARENA_DEFAULT_RESERVE_SIZE);
    if (repl->arena == 0)
    {
        return 0;
    }

    if (!jit_init(&repl->jit, JIT_DEFAULT_RESERVE_SIZE))
    {
        arena_release(repl->arena);
        MemoryZeroStruct(repl);
        return 0;
    }
    return 1;
}

// Drops the tail of an over-long line so its fragments do not arrive as if the
// user had typed them.
static void repl_discard_line_(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
}

static b32 repl_line_is_blank_(Str8 line)
{
    for (u64 i = 0; i < line.size; i += 1)
    {
        u8 c = line.str[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
        {
            return 0;
        }
    }
    return 1;
}

// A failure here prints and returns. The session outlives every bad line.
static void repl_eval_(Repl *repl, Str8 line)
{
    Temp scratch = temp_begin(repl->arena);

    Node *ast = 0;
    Str8 err  = {0};
    if (!parse_expr(repl->arena, line, &ast, &err))
    {
        printf("error: %.*s\n", Str8VArg(err));
        temp_end(scratch);
        return;
    }

    // The compiled bytes live in repl->jit, so they outlive this scratch scope.
    JitFunc func = 0;
    if (!jit_compile(&repl->jit, repl->arena, ast, &func))
    {
        printf("error: out of code space\n");
        temp_end(scratch);
        return;
    }

    i64 value = func();
    printf("%lld\n", (long long)value);

    temp_end(scratch);
}

void repl_run(Repl *repl)
{
    Assert(repl != 0 && repl->arena != 0);

    // Line editing and history want a real terminal library. fgets holds the
    // place until then.
    char line[REPL_LINE_MAX];
    for (;;)
    {
        printf("csh> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == 0)
        {
            printf("\n");
            break;
        }

        Str8 src = str8_from_cstr(line);

        // fgets stops at the buffer edge without saying so. Evaluating the
        // fragment would answer a different expression than the one typed.
        if (src.size == sizeof(line) - 1 && src.str[src.size - 1] != '\n')
        {
            printf("error: line longer than %d bytes\n", REPL_LINE_MAX - 1);
            repl_discard_line_();
            continue;
        }

        if (repl_line_is_blank_(src))
        {
            continue;
        }
        repl_eval_(repl, src);
    }
}

void repl_shutdown(Repl *repl)
{
    jit_shutdown(&repl->jit);
    arena_release(repl->arena);
    MemoryZeroStruct(repl);
}
