# Style

The conventions this template already follows. Read the base layer for worked
examples. `.clang-format` is the authority on formatting, so run it rather than
arguing with it.

## Formatting

Allman braces. This is the standard for this project. Braces go on their own
line, including for `if`, `for`, `while`, and `switch`.

Four spaces, never tabs. 100 column limit.

Every control statement gets braces, including single-statement bodies.

Pointers bind to the name: `Arena *arena`, not `Arena* arena`.

## Naming

| Kind | Convention | Examples |
| --- | --- | --- |
| Types | `PascalCase` | `Arena`, `Str8`, `App`, `Temp` |
| Platform types | `OS_` prefix | `OS_Handle`, `OS_ThreadFunc` |
| Functions | `snake_case`, module prefix | `arena_push`, `str8_concat`, `os_file_read`, `app_run` |
| Variables | `snake_case` | `start_time_us`, `capacity_new` |
| Macros | `PascalCase` | `ArrayCount`, `Min`, `Assert`, `PushStruct`, `LogInfo` |
| Constant macros | `SCREAMING_SNAKE` | `ARENA_HEADER_SIZE`, `OS_PATH_MAX` |
| Enum type | `PascalCase` | `LogLevel` |
| Enum values | `Type_Value` | `LogLevel_Info`, `LogLevel_COUNT` |
| File-private | trailing `_` | `array_grow_`, `log_min_level_`, `os_thread_entry_` |

The module prefix is the type it operates on, lowercased. Functions on `Arena`
are `arena_*`. Functions on `Str8` are `str8_*`. Module functions in `app.c` are
`app_*`.

Macros are `PascalCase` so a reader can tell at the call site that arguments may
be evaluated more than once. `Min`, `Max`, and `Clamp` all do this, so never
pass them an expression with side effects.

Use the fixed-width types from `base_types.h` everywhere. No bare `int`, `long`,
`unsigned`, or `char *` for strings. `b32` is the boolean.

## Memory

Every allocation comes from an arena, and the arena is always an explicit
parameter. No `malloc` or `free` in project code. The only calls to the OS
allocator live in `base_os.h`.

A module owns its arena. It reserves in `init`, releases in `shutdown`, and
everything in between comes from that arena. One `arena_release` frees it all.

Arenas reserve address space and commit on demand, so reserve generously.
`ARENA_DEFAULT_RESERVE_SIZE` is a gigabyte and costs nothing until touched.

Use `Temp` for anything scratch. Pushes between `temp_begin` and `temp_end` are
released at the end of the scope.

Prefer `arena_push`, which zeroes. Reach for `arena_push_no_zero` only when the
whole block is about to be overwritten.

Data lives as long as its arena. There is no per-allocation free, and never a
free of an individual pointer.

## Error handling

Functions return `b32`, where zero is failure, or a status enum when the caller
needs to distinguish causes. Values come back through output parameters.

```c
Str8 contents = {0};
if (!os_file_read(arena, path, &contents))
{
    LogError("could not read %.*s", Str8VArg(path));
    return 0;
}
```

No global error state and no `errno` equivalent. A function that fails leaves
its output parameters zeroed and the arena where it found it.

`Assert` is for programmer error, meaning broken invariants and bad arguments.
It compiles out when `BUILD_DEBUG` is 0. Never put logic with side effects
inside an `Assert`. Runtime failure that the caller could hit legitimately, such
as a missing file, returns a status instead.

## Header discipline

The base layer is single-header STB-style. Each `base_*.h` holds declarations at
the top and its implementation at the bottom behind `#ifdef BASE_IMPLEMENTATION`.
There are no `base_*.c` files. Include `base.h` and never an individual
`base_*.h`, because the include order in `base.h` is what resolves the
dependency cycle between strings, arenas, and the OS layer.

Project modules are `.h`/`.c` pairs. Declarations in the `.h`, definitions in the
`.c`. `src/app/` is the worked example to copy.

The rule for which to use: the base layer is single-header because it is
foundational, stable, and copied between projects. Project code changes daily
and gets ordinary pairs. When in doubt, write a pair.

Include guards are `#ifndef`/`#define`/`#endif` named after the file, such as
`BASE_ARENA_H`. No `#pragma once`.

Platform code belongs in `base_os.h` behind `#if OS_WINDOWS` and friends. Those
macros are always defined, so test them with `#if` and never `#ifdef`. No other
file calls an OS API directly.

## Unity build

`src/main.c` is the only file passed to the compiler. It includes
`src/unity_build.c`, which defines `BASE_IMPLEMENTATION`, includes `base/base.h`,
then includes every project `.c` file.

A new project `.c` file gets an `#include` in `unity_build.c`, in dependency
order. It never goes on the compiler command line, and the build scripts never
change.

The whole program is one translation unit, so base layer functions are not
`static`. Statics are reserved for file-private helpers, named with a trailing
underscore.

## Comments

Comments are not paragraphs. Two lines is the ceiling. The one exception is the
file-level comment at the top of a file.

If code needs a paragraph to explain, fix the code instead.

A comment states a constraint or a reason the code cannot show. It never
restates the next line.

No em dashes, and no mid-sentence asides.

## Third party

Vendored single-header libraries go in `third_party/` unmodified. Include them
from `unity_build.c`, not from base layer headers.
