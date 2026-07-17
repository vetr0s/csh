# TODO

Ordered roughly by what blocks what. `docs/spec.typ` carries the reasoning; this
is the worklist.

## Next

- [ ] **Real compile-time constants.** `X :: 100` is an immutable binding today:
      it takes a slot, is initialised at runtime, and the checker rejects
      assignment to it. Odin's `::` folds into the instruction stream and has no
      storage. Needs a constant folder. Decides whether `add :: proc(...)` can be
      an immutable binding holding a code address, or needs the real thing.
- [ ] **A block's value is its trailing expression.** A line already works this
      way. Extending it to `{}` makes `if` an expression and means C's ternary
      never needs to exist. Do it with control flow, not before.
- [ ] Types and variables are separate namespaces, and the type side is a string
      compare against `"i64"` in `check.c`. Fine with one type. A real type table
      arrives with the second one.

## Then

- [ ] Reclaim code space, or accept the leak and say so. Every line's compiled
      bytes live until the session ends. `Jit.pos` only moves forward.
- [ ] A declaration that traps still declares its name, holding zero.
      `z: i64 = 5 / 0;` errors, and `z` is then 0 rather than absent. Fixing it
      means deferring the declaration until the line has run.
- [ ] Replace the trap channel with the brief's tagged fallible values, once
      there is a type system to express them. The trap slot is a real answer,
      not a placeholder, but it is not the destination.

## Language

- [ ] More types than `i64`. This is what turns `check` into a real type
      checker, and it is where the fixed-width types from `base_types.h` should
      surface as language types.
- [ ] Strings. Needed before anything shell-shaped is possible.
- [ ] Control flow: `if`, `while`. The backend already has `cbz`, `b`, and
      forward-reference patching, which division by zero forced. Labels and
      comparison operators are what is missing.
- [ ] Functions. Needs a calling convention, a prologue and epilogue, and a
      decision about whether a function's locals live on the machine stack or in
      an arena.
- [ ] `load("file")` at the prompt, which needs strings. Open question then: a
      file is one compilation unit, so an error anywhere means none of it runs.
      Right for a script, maybe wrong for an interactive load.
- [ ] Script arguments. `$1` and `$2` become ordinary typed parameters once
      functions exist, so `vsh script.vsh a b` waits on both.

## Shell

Nothing here exists. All of it is the point of the project.

- [ ] **Decide how a command is invoked.** `ls -la` parses as `ls` minus `la`.
      This is the one that decides whether vsh is a shell or a calculator with
      ambitions, and it blocks the pipe dispatcher. Options and their costs are
      in the spec's Command Invocation chapter. A heuristic is disqualified:
      guessing wrong runs a different command than the one typed.
- [ ] `cd`, and the process-control builtins.
- [ ] External processes: `fork`, `exec`, `pipe`, `dup2`, behind an explicit
      builtin rather than implicit in the syntax.
- [ ] The hybrid pipe dispatcher: `|` between vsh functions is a call, `|` to a
      binary is a real pipe.
- [ ] Redirection.
- [ ] `exit(code)` and a separately named session exit. The brief is emphatic
      that conflating these is a bash mistake worth avoiding by construction.
      Open question: what `exit()` means inside an in-process pipe stage, which
      is a function call and not a process.
- [ ] Environment variables as a typed table, with an explicit export at the
      boundary to child processes.

## Engine

- [ ] Register allocation. Codegen is a stack machine today: every value goes
      through memory. Correct at any depth, but wasteful.
- [ ] Constant folding. `2 + 3` currently emits both loads and an add.
- [ ] x86_64 backend. `jit.c` is arm64 only and `#error`s elsewhere. The
      executable-memory layer in `base_os.h` is already written for all three
      platforms, so this is codegen work alone.
- [ ] Line editing and history. `fgets` is a placeholder. A line longer than
      `REPL_LINE_MAX` is rejected rather than truncated, which is honest but not
      good. Likely a vendored single-header library in `third_party/`.

## Robustness

- [ ] Deep nesting recurses in both `parse` and `jit` and would overflow the C
      stack. The 4095-byte line limit currently hides this. It stops hiding it
      the moment lines can be longer.
- [ ] Errors carry no source position. `Token.offset` is recorded and then
      never used. A caret under the offending token wants it.
- [ ] There are no tests. Everything so far has been verified by driving the
      binary by hand. `difr` (golden-file diffs for CLI programs) is the
      author's own tool and fits this exactly.

## Upstream

- [ ] Consider upstreaming arch detection and the executable-memory pair to
      c-project-template, once they have settled. Most C projects do not JIT, so
      arch detection is the likelier candidate of the two.
- [ ] `os_decommit` uses `MADV_DONTNEED`, which does not release pages on Darwin
      the way it does on Linux. Latent, since the arena never decommits. Noticed
      but not verified. Would be a template fix, not a vsh one.
