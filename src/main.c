// main.c - the only file passed to the compiler. Everything else arrives
// through unity_build.c.

#include "unity_build.c"

int main(int argc, char **argv)
{
    Unused(argc);
    Unused(argv);

    Repl repl;
    if (!repl_init(&repl))
    {
        LogError("repl_init failed");
        return 1;
    }

    repl_run(&repl);
    repl_shutdown(&repl);
    return 0;
}
