# vsh

An interactive, compiled, C-like shell language, loosely inspired by TempleOS's
HolyC.

vsh compiles what you type and runs it. There is no interpreter and no bytecode.
Typing `2 + 3 * 4` lexes it, parses it, resolves its names, emits arm64
instructions, writes those bytes into an executable page, and calls them as a
function. The compiler is vsh's own, written from scratch. Nothing is embedded.

## Status

Early, and not yet a shell. vsh evaluates integer arithmetic with variables that
persist across lines. There are no processes, pipes, redirection, strings,
functions, control flow, or types beyond `i64`.

That order is deliberate. The execution engine and the memory model are being
proven before any syntax is designed around them. See `docs/spec.typ` for the
whole picture and `TODO.md` for what is next.

## Build

```sh
./scripts/build_macos.sh            # debug, -> build/vsh
./scripts/build_macos.sh release    # -O2, no debug info

./scripts/build_linux.sh            # untested, see Requirements
scripts\build_windows.bat           # untested, see Requirements
```

Only `src/main.c` reaches the compiler. Everything else arrives through
`src/unity_build.c`. There is no build system to configure.

## Use

```
vsh> 2 + 3 * 4
14
vsh> i64 x = 5;
vsh> x * 3
15
vsh> x = 9
9
vsh> it + 1
10
vsh> i64 a = 100; i64 b = 100; i64 ab = a * b;
vsh> ab
10000
```

A line is any number of statements, optionally ending in an expression with no
semicolon. That trailing expression is the line's value and the only thing
printed, so the semicolon means "discard this", as `foo();` does in C. `2 + 3`
prints 5 and `2 + 3;` prints nothing.

Comments are `//` to end of line and `/* */`.

## Files

```sh
./build/vsh script.vsh
```

A file is compiled and run as one unit. Because it is a sequence of terminated
statements, it prints nothing, and because newlines are only whitespace, it is
the same grammar the prompt uses. Exit status is 0 when it ran, 1 when it could
not be read or something in it failed, 2 for misuse.

`it` holds the last expression's value. It is an ordinary entry in the symbol
table, not magic punctuation, so it reads like any other name. The name comes
from GHCi, which vsh borrows its REPL feel from. It replaces bash's `$?`.

Ctrl-D exits. There is no `exit` yet, deliberately: the goal is that command
exit and session exit are different things by construction, and that decision is
still open.

## Requirements

arm64. `src/jit/jit.c` emits arm64 and nothing else, and refuses to compile
elsewhere with an `#error` rather than failing at runtime.

Developed and tested only on arm64 macOS. The base layer implements executable
memory for Windows and Linux as well, but neither is exercised. On macOS the JIT
needs no entitlement and no codesigning.

## Layout

```
build/          compiler output, gitignored
docs/           spec.typ, the language specification
scripts/        build scripts, one per platform, plus tags.sh
src/
  base/         base layer, vendored from c-project-template
  lex/          source text to tokens
  parse/        tokens to a tree
  sym/          the session symbol table
  check/        name resolution, between parse and codegen
  jit/          tree to arm64, and executable memory
  repl/         the read, compile, run, print loop
  unity_build.c every project .c file is included here
  main.c        the only file passed to the compiler
third_party/    vendored single-header libraries
```

Data flows one way: `lex -> parse -> check -> jit -> call`. `repl` drives all of
it and owns the session.

## Base layer

`src/base/` is vendored from
[c-project-template](https://github.com/vetr0s/c-project-template) at `67630fb`
and is copied rather than depended on. It arrived in its own commit, `e8f9a7a`,
so that `git diff e8f9a7a..HEAD -- src/base/` is exactly what vsh has added to
upstream. That is what a re-sync needs.

vsh adds two things to it: architecture detection, and an executable-memory pair
(`os_reserve_exec`, `os_commit_exec`, `os_exec_write_begin`,
`os_exec_write_end`). Both may be worth upstreaming once they settle.

## Conventions

`STYLE.md` is the authority and `.clang-format` settles formatting. Every
allocation comes from an arena and no project code calls `malloc`. Read
`STYLE.md` before writing any.
