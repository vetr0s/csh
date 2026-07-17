# TODO

Ordered roughly by what blocks what. `docs/spec.typ` carries the reasoning; this
is the worklist.

## Next

- [ ] **Decide how division by zero reports.** arm64 `sdiv` yields 0 and never
      traps, so `5 / 0` is currently a confident wrong answer. This is the first
      real instance of the brief's soft-failure question, and whatever is chosen
      sets the pattern for every fallible operation after it.
- [ ] Reclaim code space, or accept the leak and say so. Every line's compiled
      bytes live until the session ends. `Jit.pos` only moves forward.

## Language

- [ ] More types than `i64`. This is what turns `check` into a real type
      checker, and it is where the fixed-width types from `base_types.h` should
      surface as language types.
- [ ] Strings. Needed before anything shell-shaped is possible.
- [ ] Control flow: `if`, `while`. Needs branches and labels in the backend.
- [ ] Functions. Needs a calling convention, a prologue and epilogue, and a
      decision about whether a function's locals live on the machine stack or in
      an arena.
- [ ] Comments in the lexer.

## Shell

Nothing here exists. All of it is the point of the project.

- [ ] `cd`, and the process-control builtins.
- [ ] External processes: `fork`, `exec`, `pipe`, `dup2`, behind an explicit
      builtin rather than implicit in the syntax.
- [ ] The hybrid pipe dispatcher: `|` between csh functions is a call, `|` to a
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
      but not verified. Would be a template fix, not a csh one.
