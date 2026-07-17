// repl.c - see repl.h.

#include "repl.h"

#include "../check/check.h"
#include "../parse/parse.h"

#include <stdio.h>

b32 repl_init(Repl *repl)
{
    MemoryZeroStruct(repl);

    repl->session = arena_alloc(ARENA_DEFAULT_RESERVE_SIZE);
    repl->scratch = arena_alloc(ARENA_DEFAULT_RESERVE_SIZE);
    if (repl->session == 0 || repl->scratch == 0)
    {
        repl_shutdown(repl);
        return 0;
    }

    if (!jit_init(&repl->jit, JIT_DEFAULT_RESERVE_SIZE))
    {
        repl_shutdown(repl);
        return 0;
    }

    sym_init(&repl->syms, repl->session);

    // Lives in the session arena because compiled code holds its address.
    repl->trap = PushStruct(repl->session, i64);

    // `it` is an ordinary symbol, so reading it needs no special case in the
    // lexer, the parser, or codegen. Only the write in repl_eval_ is special.
    // Not a constant: the prompt writes it every line.
    if (repl->trap == 0 || sym_declare(&repl->syms, Str8Lit("it"), 0).slot == 0)
    {
        repl_shutdown(repl);
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

// A failure prints and returns 0. The prompt ignores that and takes the next
// line; a file stops and makes it the exit status.
static b32 repl_eval_(Repl *repl, Str8 source)
{
    Temp scratch = temp_begin(repl->scratch);

    Node *ast = 0;
    Str8 err  = {0};
    if (!parse_line(repl->scratch, source, &ast, &err))
    {
        printf("error: %.*s\n", Str8VArg(err));
        temp_end(scratch);
        return 0;
    }

    // Declares into the session arena, so the slot outlives this scope even
    // though the error message does not.
    if (!check_resolve(repl->scratch, &repl->syms, ast, &err))
    {
        printf("error: %.*s\n", Str8VArg(err));
        temp_end(scratch);
        return 0;
    }

    JitFunc func = 0;
    if (!jit_compile(&repl->jit, repl->scratch, ast, repl->trap, &func))
    {
        printf("error: out of code space\n");
        temp_end(scratch);
        return 0;
    }

    *repl->trap = TrapKind_None;
    i64 value   = func();

    // The value is whatever the trapping operation left behind, so it is not
    // worth printing. A name declared on a trapping line stays declared and
    // holds zero, because check_resolve created it before any of this ran.
    if (*repl->trap != TrapKind_None)
    {
        printf("error: %s\n", jit_trap_message((TrapKind)*repl->trap));
        temp_end(scratch);
        return 0;
    }

    // Only a trailing expression with no semicolon has a value worth showing.
    // A line of statements, and so a whole loaded file, prints nothing.
    if (NodeBlockHasValue(ast))
    {
        // Looked up rather than cached, because redeclaring `it` makes a new
        // slot and the prompt should write the one the next line will read.
        i64 *it = sym_lookup(&repl->syms, Str8Lit("it")).slot;
        Assert(it != 0);
        *it = value;
        printf("%lld\n", (long long)value);
    }

    temp_end(scratch);
    return 1;
}

// The file is one unit, not a line at a time, because newlines are only
// whitespace and the grammar already spans them. A file is the same statement
// sequence a prompt takes, minus the trailing expression, so it prints nothing.
b32 repl_run_file(Repl *repl, Str8 path)
{
    Temp scratch = temp_begin(repl->scratch);

    Str8 source = {0};
    if (!os_file_read(repl->scratch, path, &source))
    {
        LogError("cannot read %.*s", Str8VArg(path));
        temp_end(scratch);
        return 0;
    }

    // Pushed before repl_eval_ opens its own scope on the same arena, so it
    // survives that scope closing and is released by the temp_end below.
    b32 result = repl_eval_(repl, source);

    temp_end(scratch);
    return result;
}

void repl_run(Repl *repl)
{
    Assert(repl != 0 && repl->session != 0);

    // Line editing and history want a real terminal library. fgets holds the
    // place until then.
    char line[REPL_LINE_MAX];
    for (;;)
    {
        printf("vsh> ");
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
    arena_release(repl->session);
    arena_release(repl->scratch);
    MemoryZeroStruct(repl);
}
