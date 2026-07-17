#set document(title: "vsh Language Specification", author: "Nathan Tebbs")
#set page(paper: "us-letter", margin: (x: 1.25in, y: 1in), numbering: "1")
#set text(font: "New Computer Modern", size: 11pt)
#set heading(numbering: "1.1.")
#show heading.where(level: 1): it => {
  v(1.2em)
  text(size: 14pt, weight: "bold", it)
  v(0.4em)
}
#show heading.where(level: 2): it => {
  v(0.8em)
  text(size: 12pt, weight: "bold", it)
  v(0.2em)
}
#show heading.where(level: 3): it => {
  v(0.6em)
  text(size: 11pt, weight: "semibold", style: "italic", it)
  v(0.1em)
}
#show raw.where(block: false): it => box(
  fill: rgb("#f2f2f2"), inset: (x: 3pt, y: 0pt), outset: (y: 3pt), radius: 2pt, it,
)
#show raw.where(block: true): it => block(
  fill: rgb("#f7f7f7"), inset: 8pt, radius: 3pt, width: 100%, it,
)

// Status of each claim in this document. The point is that a reader can tell
// what vsh does from what vsh intends, which are very different sizes today.
#let done = box(
  fill: rgb("#e6f4ea"), inset: (x: 5pt, y: 2pt), radius: 3pt,
  text(size: 7.5pt, weight: "bold", fill: rgb("#1e6b34"), "BUILT"),
)
#let planned = box(
  fill: rgb("#e8eef9"), inset: (x: 5pt, y: 2pt), radius: 3pt,
  text(size: 7.5pt, weight: "bold", fill: rgb("#1a4a8a"), "DECIDED"),
)
#let undecided = box(
  fill: rgb("#fdf0e3"), inset: (x: 5pt, y: 2pt), radius: 3pt,
  text(size: 7.5pt, weight: "bold", fill: rgb("#8a5a1a"), "OPEN"),
)
#let superseded = box(
  fill: rgb("#fdeaea"), inset: (x: 5pt, y: 2pt), radius: 3pt,
  text(size: 7.5pt, weight: "bold", fill: rgb("#8a1a1a"), "SUPERSEDED"),
)

// ─────────────────────────────────────────────
//  Title Page
// ─────────────────────────────────────────────
#align(center)[
  #v(3em)
  #text(size: 28pt, weight: "bold")[vsh]
  #v(0.5em)
  #text(size: 14pt)[Language Specification]
  #v(0.5em)
  #text(size: 11pt, fill: rgb("#666666"))[
    Nathan Tebbs · Draft · #datetime.today().display()
  ]
  #v(1em)
  #line(length: 60%)
  #v(1em)
  #text(size: 12pt, style: "italic")[An interactive, compiled, C-like shell language. You type C, it compiles to native code, it runs. The compiler is its own.]
]

#pagebreak()

#outline(indent: 1.5em)

#pagebreak()

// ─────────────────────────────────────────────
= About This Document

This is a sketch, not a standard. vsh is early enough that most of what follows
is intention rather than behaviour, and the document's main job is to keep those
two apart. Every section carries one of four marks:

#v(0.5em)
#table(
  columns: (auto, 1fr),
  stroke: none,
  align: (left, left),
  row-gutter: 0.7em,
  done, [Built and exercised. The code does this today, and it is the intended design.],
  planned, [Decided, with a reason on record, but not written yet.],
  superseded, [The code does this today, and a decision on record replaces it. Do not build on it. See the next chapter.],
  undecided, [Genuinely unsettled. Do not assume. These are the interesting parts.],
)
#v(0.5em)

Where a decision was made for a reason, the reason is written down. Reversing a
decision is cheap; rediscovering why it was made is not.

This document describes the language vsh is meant to be. Where the code has not
caught up, the section says so and the next chapter lists every gap in one
place. Nothing here is aspiration wearing a BUILT badge.

The worklist lives in `TODO.md`. This document explains, that one enumerates.

// ─────────────────────────────────────────────
= Where The Code Lags

The syntax below was decided after the implementation was written, so the two
disagree. This is the whole list. Each row is tracked in `TODO.md`.

#v(0.5em)
#table(
  columns: (1fr, 1fr),
  stroke: 0.5pt + rgb("#cccccc"),
  inset: 7pt,
  [*The language*], [*What the code does today*],
  [`x: i64 = 100;`], [`i64 x = 100;`],
  [`x := 100;` infers], [nothing; the type is always written],
  [`X :: 100;` is a constant], [nothing; there are no constants],
  [assignment is a statement], [assignment is an expression, so `1 + (a = 7)` is 8],
  [a block's value is its trailing expression], [only a *line* has that rule],
)
#v(0.5em)

Everything else in this document is marked accurately. The lag exists because
the syntax question was worked out on paper before code chased it, which is the
cheap order to do it in.

// ─────────────────────────────────────────────
= Overview

== What vsh Is

vsh is a statically typed, imperative, compiled language with a C-like feel,
driven from a REPL, intended to become a shell you live in rather than only
script from.

Typing an expression compiles it. There is no interpreter and no bytecode. The
text is lexed, parsed, name-resolved, emitted as arm64 machine code, written
into an executable page, and called as a function. The value comes back in a
register.

== Where It Comes From

Three inspirations, each contributing one thing:

#table(
  columns: (auto, 1fr),
  stroke: 0.5pt + rgb("#cccccc"),
  inset: 7pt,
  [*bash*], [Pipes, process control, and the shell as a place you live in.],
  [*GHCi*], [The REPL loop. Type an expression, get a value, with a compiler catching mistakes before anything runs. The loop only. Functional programming is explicitly not a goal.],
  [*C*], [A manual memory model, no hidden runtime, direct to the metal, and the appeal of building your own tools rather than gluing other people's together.],
)

TempleOS's HolyC is the closest existing thing: a shell whose REPL was a live C
compiler. vsh takes that pitch and applies it deliberately, with answers to the
things bash and raw C get wrong.

== Non-Goals

- *Functional programming.* The REPL borrows from GHCi. The language does not.
- *A hidden runtime.* No garbage collector, no exceptions, no implicit
  allocation.
- *Embedding someone else's compiler.* Considered and rejected; see below.

== On Not Using tinycc

The original plan embedded tinycc through `libtcc`. It is not used, for two
independent reasons.

The first is the point of the project. Writing the compiler is the work.
Embedding libtcc would put someone else's code exactly where vsh's belongs, and
would turn the interesting problems, which are persistence, arena scoping, and
compile granularity, into problems of fighting libtcc's model instead of
designing one.

The second is mechanical. Homebrew deprecates tcc as unsupported upstream and
disables it on 2026-09-16. Stable is 0.9.27, tagged 2017, three years before
Apple Silicon shipped. It publishes no bottle for any platform, and its formula
records build failures. It was a poor foundation independent of taste.

// ─────────────────────────────────────────────
= Execution Model

#done

== The Pipeline

One line becomes one function.

```
lex  ->  parse  ->  check  ->  jit  ->  call
```

`lex` turns bytes into tokens. `parse` turns tokens into a tree. `check`
resolves every name to a storage address. `jit` walks the tree emitting arm64
and installs the bytes in executable memory. `repl` calls the result and prints
what comes back.

Data flows one way. No stage reaches backward.

== Executable Memory

The backend needs pages it can both write and jump to. On Apple Silicon that is
constrained, and the constraints were measured rather than assumed:

#table(
  columns: (1fr, auto),
  stroke: 0.5pt + rgb("#cccccc"),
  inset: 7pt,
  [*Question*], [*Answer*],
  [Does `MAP_JIT` work unsigned, with no entitlement?], [Yes],
  [Can a live code page be patched and re-called?], [Yes],
  [Does reserve-then-commit compose with `MAP_JIT`?], [Yes],
  [Can ordinary reserved memory be made executable later?], [No, `EACCES`],
  [Is the write-protect toggle per-mapping?], [No, per-thread],
)

Two of these shaped the code. Because `MAP_JIT` must be passed at `mmap` time
and cannot be added by `mprotect` afterward, executable memory is a separate
reservation rather than a flag on the existing one. Because the write toggle is
thread-wide, it cannot be modelled as a nestable scope: `jit.c` builds
instructions into a scratch array and installs them in a single write rather
than toggling as it walks the tree.

That a live page can be patched and re-called is the mechanism by which
redefinition will eventually work.

== Codegen

Codegen is a stack machine. Every node leaves its value on the machine stack,
and a binary operator pops two and pushes one. Push and pop move the stack
pointer by 16 rather than 8, because arm64 requires `sp` to stay 16-byte
aligned.

This emits instructions a register allocator would not. It is correct at any
expression depth, needs no allocation pass, and is easy to read. Registers come
later, and `TODO.md` tracks it.

Integer literals load with `movz` and up to three `movk`. A negative value fills
all four halfwords and so costs four instructions.

The backend has `cbz` and `b`, and patches a branch's offset once its target is
known. Division by zero forced that machinery; control flow will reuse it.

== Compile Granularity

#undecided

Today one input line is one function, compiled whole. HolyC compiled each line
as its own function too. Whether vsh keeps that, or moves to explicit
`{}`-delimited blocks, is unsettled and interacts with control flow and
functions. It is deliberately not being answered before those exist.

// ─────────────────────────────────────────────
= Memory Model

#done

== Two Arenas, Two Lifetimes

A REPL has exactly two lifetimes, so the session has exactly two arenas.

#table(
  columns: (auto, 1fr),
  stroke: 0.5pt + rgb("#cccccc"),
  inset: 7pt,
  [*Session*], [Symbols, their names, and their storage. Never popped. Lives until the session ends.],
  [*Scratch*], [Tokens, tree, and instruction buffer. Popped after every line, because none of it means anything once the line has run.],
)

Both are arenas from the base layer: they reserve a gigabyte of address space
and commit pages only as the bump pointer reaches them. Reserving costs nothing
until touched.

There is no `malloc` in vsh, no `free`, and no per-allocation lifetime. Raw
manual allocation is unworkable in a REPL, where a variable declared on line 1
and read on line 50 makes "did I free this?" impossible to answer by hand. A
garbage collector would betray the no-hidden-runtime goal. Arena scoping is the
middle ground, and it is what the base layer already provides.

== Why State Survives A Line

This is the problem the project is really about, so it is worth stating plainly.

A normal one-shot compile throws away all state when the process exits. A shell
cannot. The answer is that *vsh owns the storage, not the compiler.*

`sym.c` gives every variable a slot in the session arena. Because that arena is
never popped, the slot's address is fixed the moment the name is declared.
Codegen then bakes the address in as an immediate and reaches the variable
through a plain load of a constant address. A line compiled minutes later, as a
separate function, in a different page, loads from the same address and sees the
same value.

Nothing is shared with the compiler because the compiler owns nothing.

== Slots Never Move

The symbol table hands out slot pointers, never `Symbol` pointers. The `Symbol`
array is a dynamic array, so it moves when it grows, and a `Symbol *` taken
before a growth would dangle. Slots are pushed individually and never move.

This is verified: declaring fifty variables forces roughly four reallocations of
the array, and the first variable still reads correctly afterward.

Names are copied into the session arena at declaration. A token's name borrows
the line buffer, and the next prompt overwrites it.

// ─────────────────────────────────────────────
= Lexical Structure

#done

#table(
  columns: (auto, 1fr),
  stroke: 0.5pt + rgb("#cccccc"),
  inset: 7pt,
  [*Integer*], [One or more decimal digits. Value must fit `i64`; overflow is a diagnostic, never a wrap.],
  [*Name*], [`[A-Za-z_][A-Za-z0-9_]*`],
  [*Keyword*], [`i64`. The only one.],
  [*Operators*], [`+` `-` `*` `/` `=`],
  [*Punctuation*], [`;` `(` `)`],
  [*Whitespace*], [Space, tab, carriage return, newline. Separates tokens, otherwise ignored.],
  [*Comments*], [`//` to end of line, and `/* */`, which does not nest. A `/*` that never closes is a diagnostic rather than a silent run to end of file.],
)

Whitespace and comments are skipped together, because both are only gaps between
tokens. A lone `/` is division, so the two-character lookahead is what tells
`10 / 2` from `10 // 2`.

Newlines are whitespace and nothing more. The grammar spans them, which is why a
file needs no separate parser.

There are no string or character literals. A byte that begins no token is a
diagnostic, and the lexer always advances past it, so a bad byte cannot loop the
parser.

// ─────────────────────────────────────────────
= Grammar

#done

```
line    := stmt* expr?
stmt    := decl | assign | expr ';' | ';'
decl    := NAME ':' type? ('=' | ':') expr ';'
assign  := NAME '=' expr ';'
expr    := addsub
addsub  := term (('+' | '-') term)*
term    := unary (('*' | '/') unary)*
unary   := '-' unary | primary
primary := INT | NAME | '(' expr ')'
```

Recursive descent, two tokens of lookahead. Two rather than one because `NAME
':'` is what marks a declaration and `NAME '='` an assignment, and both start
the same way. The parse stays context-free either way, which is the point of the
whole chapter below.

The first error wins and the rest of the parse unwinds without reporting, so a
reader sees the cause rather than its consequences.

Trailing input is an error. Without that rule `1 2` would quietly evaluate to 1,
which is the class of silent wrongness vsh exists to avoid.

The parser still implements the older C-style form, `'i64' NAME '=' expr ';'`,
with assignment inside `expr`. See "Where The Code Lags".

== What The Semicolon Is For

#done The rule is built. The declarations below are written in the decided
syntax, not the syntax the code accepts today.

A line is any number of statements, optionally ending in an expression with *no*
semicolon. That trailing expression is the line's value and the only thing the
prompt prints.

```
vsh> x := 100; y := 100; xy := x * y;
vsh> xy
10000
vsh> a := 5; a * 2
10
vsh> 2 + 3 * 4
14
vsh> 2 + 3 * 4;
vsh>
```

So the semicolon means "discard this value", which is exactly what it already
means in C, where `foo();` throws away what `foo` returned. Until statements
could be sequenced, the semicolon only terminated a declaration and carried no
weight, which made it decoration. Now it earns its place.

This rule is also what makes files work. A file is a sequence of statements, all
terminated, so loading one runs it and prints nothing. A prompt is the same
grammar with a trailing expression on the end.

An empty statement is allowed, as in C, so a stray `;;` costs nothing.

Statements chain through a `next` pointer rather than nesting, and both the
checker and codegen walk that chain with a loop. A file of ten thousand
statements therefore costs one stack frame rather than ten thousand.

// ─────────────────────────────────────────────
= Types

#done for `i64`. #undecided for everything else.

`i64` is the only type. It is 64-bit, signed, two's complement.

The intent is that the fixed-width types the base layer already uses, `i8`
through `u64`, `f32`, `f64`, and `b32`, surface as language types, and that
`check.c` grows from a name resolver into a type checker to match.

Widths are written, always. There is no `int` whose size depends on the machine,
because the base layer already banned bare `int` in its own code and a language
that compiles to a fixed architecture has no excuse for the vagueness. Odin
keeps a platform-sized `int` alongside the explicit widths; vsh should not.

#undecided Implicit conversion. C's integer promotions are a mistake worth not
repeating by reflex, and the alternative, an explicit cast at every narrowing, is
a real cost at a prompt. Not decided, and it does not need to be until there is a
second numeric type.

// ─────────────────────────────────────────────
= Statements and Expressions

== Declaration

#superseded The code writes the type first, C-style. The form below replaces it.

The type follows the name, after a colon, as in Odin and Pascal.

```odin
x: i64 = 100;
```

A declaration is a *statement*, so it prints nothing and does not move `it`.

The initialiser is resolved before the name is declared. That makes
`x: i64 = x + 1;` read the old `x`, and makes `y: i64 = y;` an error rather than
a read of storage that was created a moment earlier and never written.

=== Why Not C's Order

Not because it reads better, though it does. Because C's order is not
context-free, and adopting it would cost vsh the pipeline it already has.

The parser today decides declaration from expression on a single token:

```c
if (parser.token.kind == TokenKind_KeywordI64)
```

That works only while every type name is a keyword. The moment a user can name a
type, `Foo * x;` is ambiguous: a pointer declaration, or `Foo` multiplied by
`x`? C settles it by having the parser ask the symbol table what `Foo` is. That
is the lexer hack, and it is why C is context-sensitive rather than
context-free.

vsh's flow is `lex -> parse -> check`, and `check` exists precisely so `parse`
never needs to know what a name means. `parse.c` includes `lex.h` and nothing
else. C's declaration order would force it to depend on `sym`, and the one-way
flow would be gone, traded for a 1972 mistake.

`NAME ':'` cannot be ambiguous. Two tokens of lookahead, no symbol table, and
`parse` stays ignorant of names forever.

C's declarator syntax is also broken on its own terms, which is worth saying out
loud since "C-like" was the original brief. `int *a, b;` declares one pointer and
one int. `int *a[10]` and `int (*a)[10]` are different types read inside out.
Function pointer syntax is unreadable by consensus. Declaration-reflects-use did
not survive contact with reality, and there is no reason to inherit it out of
sentiment.

== Inference and Constants

#planned Nothing here is built.

One form, `name : type = value`, with two knobs. Omit the type and it is
inferred. Swap the `=` for a `:` and it is a constant.

```odin
x: i64 = 100;    // explicit type, variable
x := 100;        // inferred, variable
X :: 100;        // inferred, constant
X: i64 : 100;    // explicit type, constant
```

Four meanings, one rule, nothing to memorise. This is Odin's scheme and it is
taken wholesale because it is coherent.

Inference is not dynamism. `x := 100` has exactly one type, fixed at the
declaration, checked at compile time. It just is not written twice. That matters
at a prompt, where `x := 100` is the thing typed all day and `x: i64 = 100` is
the thing typed when the type is the point.

Functions fall out of the same rule, as a constant of procedure type:

```odin
add :: proc(a: i64, b: i64) -> i64 { a + b }
```

which matters more than it looks. The brief says a command *is* a function, so
`$1` and `$2` stop being bash's positional string soup and become named, typed
parameters of a `proc`. The declaration syntax and the shell's argument model
are the same decision.

== Assignment

#superseded The code makes assignment an expression. It becomes a statement.

```odin
x = 9;
```

Assignment is a *statement*. It has no value, so it cannot appear inside an
expression.

That is a deliberate reversal. Assignment is an expression in the code today,
and the reason on record was "as in C". C is no longer the authority, and this
is the first thing that fails once it stops being one.

What the change costs: `a = b = 1` chaining, and `1 + (a = 7)` evaluating to 8.
Nobody wants the second, and the first is not missed by anyone who has lived
without it.

What it buys: `if (x = 5)` cannot be written. That is one of the most famous bug
classes in C, and Go and Odin both make assignment a statement specifically to
delete it. Deleting a bug class by construction, rather than by warning about
it, is the same move as giving command exit and session exit different names.

`:=` declares and `=` assigns, so the two are distinct on sight as well as in
the grammar.

Only a name may sit left of `=`. Assigning to an undeclared name is an error;
declaration is the only thing that creates a name.

== Redeclaration

#done The behaviour is built. The examples below use the decided syntax.

Redeclaring a name makes a *new slot* and shadows the old one. The old slot
stays alive.

```odin
x := 5;
x := 9;   // a new binding, not an overwrite
x         // 9
```

Three readings were possible. Updating the slot in place was the old behaviour,
and it was the only sensible one while redeclaring was the only way to change a
value. Erroring was rejected because retyping a declaration is a normal way to
work at a prompt.

Shadowing wins for two reasons. The first is that it is the only option that
survives a second type: `x: i64` and, later, `x: f64` cannot share storage, so
an in-place update is not even expressible once types exist.

The second is that its apparent cost is not a cost. Code compiled earlier holds
the *old* address and keeps reading the old slot after a redeclaration. That
looks like a trap, but it is exactly lexical scoping, and it is what GHCi does:
a function defined before a rebinding goes on seeing the binding it was defined
against. Baking addresses into compiled code gives that behaviour for free
rather than making it a bug to work around.

Nothing observable follows from this yet, because a compiled line is called once
and thrown away. It starts to matter the moment functions exist, which is why it
was worth settling first.

`sym_lookup` therefore scans backward, so the newest declaration of a name wins,
and shadowed entries stay in the table because code still points at their slots.

Once `:=` exists, shadowing and mutation stop looking alike. `x := 9` makes a new
binding and `x = 9` changes the one that is there, and the reader can tell which
happened without knowing whether `x` already existed. Under the C form both are
spelled with `=` and only the leading type tells them apart.

== The Last Value

#done

`it` is the value of the last expression.

The original plan called this `$?`, kept from bash. The requirement was that it
be a real typed variable rather than bash's stringly-typed magic. GHCi, which
vsh already borrows its REPL feel from, calls this `it`, and taking that name
satisfies the requirement better than keeping `$?` would: `it` is an ordinary
entry in the symbol table, so no special case reaches the lexer, the parser, or
codegen. Only the REPL's assignment to it is special.

It is declared at startup like any other name, and it can be read, assigned, and
even redeclared, all without any of those paths knowing it is special. The
prompt looks its slot up rather than caching it, because redeclaring `it`
shadows it with a new slot and the prompt should write the binding the next line
will read.

Bash's other magic is answered the same way. `$1` and `$2` become ordinary typed
function parameters, once functions exist, because a command in vsh is just a
function.

== Blocks Are Expressions

#planned The rule holds for a line already. Extending it to `{}` is not built.

A block's value is its trailing expression, the one with no semicolon. That is
the rule a *line* already follows, and there is no reason for `{}` to follow a
different one.

```odin
y := if x > 0 { 1 } else { -1 };
```

This is Rust's rule, and worth being honest about how vsh got here: it was not
copied. It fell out of making files run silently, where a sequence of terminated
statements has no value and a bare trailing expression does. Arriving at an
existing language's rule from an unrelated direction is reasonable evidence the
rule is right.

Generalising it pays for itself immediately. `if` as an expression means C's
ternary never needs to exist, which is one more piece of C syntax not inherited
by reflex. It also means the prompt, a file, and a block are three uses of one
idea rather than three rules.

The `{}` form is not built, because there is no control flow yet. The decision is
recorded now so that control flow is designed around it rather than retrofitted.

// ─────────────────────────────────────────────
= Command Invocation

#undecided Nothing is decided. This is the question that decides whether vsh is
a shell or a calculator with ambitions.

== The Problem

```
ls -la
```

In an expression language, that is `ls` minus `la`. It is a real ambiguity, not
a parsing difficulty, and no amount of cleverness dissolves it. Every C-like
shell meets this wall and either commits to an answer or quietly stops being a
shell.

The constraint is not negotiable from either side. If `ls -la` cannot be typed,
nobody lives in vsh, and living in it is the entire point. If `ls("-la")` is the
only spelling, vsh is a scripting language with a REPL, which already exists and
is not interesting.

== What Is Disqualified

A heuristic. Deciding by whether a name resolves to a command, or whether spaces
surround the `-`, or what happened to parse cleanly. Guessing wrong means
silently running a different command than the one typed, which is precisely the
failure this document is organised against. `5 / 0` reports rather than answers
zero for the same reason.

== The Real Options

#table(
  columns: (auto, 1fr),
  stroke: 0.5pt + rgb("#cccccc"),
  inset: 7pt,
  [*Parens always*], [`ls("-la")`. Unambiguous, no new syntax, one grammar. Nobody would live in it, which forfeits the goal rather than meeting it.],
  [*A sigil*], [`!ls -la`, or some other mark, meaning "the rest of this line is shell words, not an expression". Honest, explicit, cheap to parse, and a constant reminder that you are leaving the typed world. Costs one character on the most common thing typed.],
  [*Bare words at the prompt only*], [The prompt takes shell words, files take expressions. Terse where terseness matters. But it gives vsh two grammars and makes a session untranslatable to a script, which is the split the semicolon rule just worked to avoid.],
  [*HolyC's half-answer*], [Parenless calls at the top level, so `Dir;` works but `Dir("*.c")` still needs parens for arguments. Solves the no-argument case, which is not the case that matters.],
)

== What It Touches

This is not an isolated syntax choice, which is why it gets its own chapter
rather than a bullet.

It decides how the hybrid pipe reads, since `|` has to sit between things whose
spelling is settled. It decides whether `$1` and `$2` really can be ordinary
typed parameters, because that only works if a command is spelled like a
function. And it decides whether a prompt session can be pasted into a file and
run, which is the property that makes a shell teachable.

Types, inference, and blocks can be built without answering this. The pipe
dispatcher cannot.

// ─────────────────────────────────────────────
= Error Handling

== At the Prompt

#done

A failure prints and the session continues. There is no exception, no unwinding,
and no hidden control flow. Every stage reports through a return value, and the
REPL prints the message and takes the next line.

This is already load-bearing: a bad line never damages the session, and the
symbol table and every prior value survive it.

== Silent Wrongness Is The Enemy

#planned

The one thing vsh must not do is confidently answer a different question than
the one asked. This is the failure bash normalises, with its ignored exit codes
and its unquoted expansions.

Three cases exist so far and were treated as bugs, not curiosities:

- An integer literal too large for `i64` is a diagnostic, not a wrap.
- A line longer than the input buffer is rejected. It used to be silently
  truncated, which meant a 6001-byte expression printed a confident, wrong
  `2048`. Truncation is now detected and the rest of the line discarded.
- Division by zero reports, rather than quietly yielding the zero that arm64
  hands back. See below.

== Division By Zero

#done

`5 / 0` reports an error and the session lives.

```
vsh> 5 / 0
error: division by zero
vsh> 42
42
```

arm64 `sdiv` yields zero for a zero divisor and never traps, so without a check
this was a confident wrong answer, which is the one thing vsh must not do.

Three answers were possible. Trapping was rejected: it is loud and cheap, but a
shell must not die because you typed `5 / 0`. Returning a tagged value is the
brief's soft-failure design and is the eventual destination, but it needs a type
system to express and so is not affordable yet. A runtime check is what is
affordable now, and it is a real answer rather than a placeholder.

=== The Trap Channel

The generated code takes a `trap` slot, an ordinary `i64` in the session arena
whose address is baked into the code like any variable's. Division emits a
branch around itself:

```
    cbz   x1, fail        // divisor is zero
    sdiv  x0, x0, x1
    b     done
fail:
    mov   x2, #TrapKind_DivideByZero
    mov   x3, #&trap
    str   x2, [x3]
    mov   x0, #0
done:
```

The prompt clears the slot, calls, and reads it back. A non-zero slot means the
value is meaningless and an error is printed instead.

The trap path *falls through* rather than returning. Returning from the middle
of an expression would strand whatever operands the surrounding expression had
already pushed, and balancing that would need a prologue that nothing else here
wants. Carrying on with a zero costs nothing, because the caller throws the
value away.

This is the first thing in vsh that needed branches, so the backend grew `cbz`,
`b`, and the patching that fills a branch's offset in once its target is known.
Control flow needs all of that anyway.

=== Consequences

A declaration that traps leaves its name declared, holding zero, because
`check.c` created the name before any code ran. `z: i64 = 5 / 0;` reports the
error, and `z` afterwards is 0 rather than absent. That is a wart. Fixing it
means deferring the declaration until the line has run, which is a larger change
than it looks.

The check is genuinely at runtime, not folded at compile time. A zero divisor
that only appears at runtime, as in `1000 / (5 - 5)`, is caught the same way.

== Fallible Operations, Generally

#planned

The intent, not yet built: a function that can fail returns a tagged value
rather than setting a global. There is no `errno`. At the prompt a failure
prints inline and the session lives. The compiler should ideally warn when a
fallible result is used without being checked.

#undecided Whether that is a hard error or a warning is open. It decides how the
REPL actually feels, which is not something to settle on paper.

== Exit

#planned for the shape. #undecided for the semantics.

Two mechanisms, deliberately named differently, because bash conflating them
confuses people regularly.

#table(
  columns: (auto, 1fr),
  stroke: 0.5pt + rgb("#cccccc"),
  inset: 7pt,
  [`exit(code)`], [Ends the current command or expression. Not the session. Should propagate up a pipe roughly the way process exit does in bash.],
  [`exit_shell()`], [Ends the interactive session. Deliberately harder to type by accident.],
)

#undecided What `exit()` means inside an in-process pipe stage is the open
question. Such a stage is a function call, not a process, so there is nothing to
exit. Whether it unwinds only that function, or emulates real process-exit
semantics despite no process existing, has to be settled before the pipe
dispatcher is written.

Nothing here is built. Ctrl-D ends the session today, and that is all.

// ─────────────────────────────────────────────
= Processes and Pipes

#planned. None of this exists.

== Hybrid Pipes

`|` means two different things depending on what is on each side.

#table(
  columns: (auto, 1fr),
  stroke: 0.5pt + rgb("#cccccc"),
  inset: 7pt,
  [*vsh to vsh*], [An in-process function call. Typed, fast, no serialisation, no `fork`, no `exec`.],
  [*vsh to a binary*], [A real pipe, through an explicit builtin such as `sh("grep foo")`, doing `fork`, `exec`, `pipe(2)`, and `dup2` underneath.],
)

The split keeps the fast typed path fast without losing the ability to call any
Unix tool. A pure in-process design would lose the second; a pure process design
would lose the first.

Routing to a binary is explicit rather than inferred. Guessing which one the
user meant is the kind of magic vsh is trying not to have.

== Environment

#planned

Environment variables have to exist, because child processes expect them. Inside
vsh they should be a typed table, with an explicit builtin to export a value out
to a child, rather than bash's untyped ambiently-inherited string soup.

// ─────────────────────────────────────────────
= Files

#done

```sh
vsh script.vsh    # runs it, prints nothing, exits 0
vsh               # the prompt
```

A file is compiled and run as *one unit*, not a line at a time. Nothing about
the language had to change to allow it. Newlines were already only whitespace,
the grammar already spanned them, and the semicolon rule already meant a
sequence of statements produces no output. A file is simply the prompt's grammar
without the trailing expression.

The whole file is read onto the scratch arena with `os_file_read` and handed to
the same evaluator a line goes to. Ten thousand statements compile and run in
about twelve milliseconds, and cost one stack frame, because statements chain
and both walkers loop.

A file gets an exit status, because something other than a person is reading it:
0 when it ran, 1 when it could not be read or something in it failed, 2 for
misuse. The prompt has no such thing, and no single line can end it.

#undecided A file is one compilation unit, so an error anywhere means none of it
runs. That is the right default for a script. It is not obviously right for a
`load` at an interactive prompt, where running the good half might be wanted.
That question can wait for `load`, which needs strings first.

// ─────────────────────────────────────────────
= Implementation Map

#done

Where to look. Data flows top to bottom.

#table(
  columns: (auto, 1fr),
  stroke: 0.5pt + rgb("#cccccc"),
  inset: 7pt,
  [`src/base/`], [Vendored from c-project-template at `67630fb`. Arenas, strings, arrays, logging, and the platform layer. vsh adds architecture detection and the executable-memory pair.],
  [`src/lex/`], [Bytes to tokens. Allocates nothing; tokens borrow the source.],
  [`src/parse/`], [Tokens to a tree, on a caller's arena. Recursive descent.],
  [`src/sym/`], [The session symbol table. Owns names and slots. Hands out slot pointers only.],
  [`src/check/`], [Name resolution. Fills each node's slot so codegen cannot fail on a name. Where the type checker goes.],
  [`src/jit/`], [Tree to arm64, and the executable code arena. arm64 only; `#error`s elsewhere.],
  [`src/repl/`], [Owns the session: both arenas, the symbol table, the code arena, and `it`. Drives the pipeline.],
)

The whole program is one translation unit. `src/main.c` is the only file the
compiler sees; everything else arrives through `src/unity_build.c`, in
dependency order.

`STYLE.md` is the authority on conventions and is not optional reading.

// ─────────────────────────────────────────────
= Open Questions, Collected

The things that are genuinely undecided, gathered from above. These are where
the design work actually is.

+ *How a command is invoked.* `ls -la` parses as subtraction. The largest one,
  and the only one that decides whether vsh is a shell. It has its own chapter.
+ *Unchecked fallible results.* Hard error or warning?
+ *Tagged fallible values*, which need a type system, and which should eventually
  replace the trap channel that division by zero introduced.
+ *A declaration that traps* still declares its name, holding zero.
+ *`exit()` inside an in-process pipe stage.* There is no process to exit.
+ *Compile granularity.* One line as one function, or explicit blocks? Now
  entangled with blocks being expressions.
+ *Reclaiming code space.* Compiled bytes currently live for the whole session.
+ *Implicit conversion*, once there is more than one type.

The brief these came from was explicit that several should be worked out
*through* prototyping rather than settled on paper. That still holds. This
document records the tension, not a verdict.

Syntax was the exception. It was settled on paper deliberately, because the cost
of changing a declaration form is a day of editing now and a rewrite later, and
because the reason for choosing it was architectural rather than aesthetic.
