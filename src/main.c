// main.c - the only file passed to the compiler. Everything else arrives
// through unity_build.c.

#include "unity_build.c"

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        LogError("usage: vsh [file]");
        return 2;
    }

    Repl repl;
    if (!repl_init(&repl))
    {
        LogError("repl_init failed");
        return 1;
    }

    // A file gets an exit status, because something else is reading it. The
    // prompt has a person reading it instead, and never fails as a whole.
    int status = 0;
    if (argc == 2)
    {
        status = repl_run_file(&repl, str8_from_cstr(argv[1])) ? 0 : 1;
    }
    else
    {
        repl_run(&repl);
    }

    repl_shutdown(&repl);
    return status;
}
