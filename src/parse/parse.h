// parse.h - tokens to a tree.
//
// The grammar, tightest binding last:
//
//     line    := stmt* expr?
//     stmt    := decl | expr ';' | ';'
//     decl    := 'i64' IDENT '=' expr ';'
//     expr    := IDENT '=' expr | addsub
//     addsub  := term (('+' | '-') term)*
//     term    := unary (('*' | '/') unary)*
//     unary   := '-' unary | primary
//     primary := INT | IDENT | '(' expr ')'
//
// A line is any number of statements, optionally ending in an expression with
// no semicolon. That trailing expression is the line's value and the only thing
// the prompt prints, so the semicolon is what says "discard this", exactly as
// `foo();` does in C. A file of statements therefore runs silently.
//
// Parsing always yields a Block, even for a bare expression, so the caller has
// one shape to handle.
//
// Assignment is an expression, as in C, so it is the lowest precedence, binds
// to the right, and has the assigned value. Only a name may sit left of '='.

#ifndef PARSE_H
#define PARSE_H

#include "../base/base.h"

typedef enum NodeKind NodeKind;
enum NodeKind
{
    NodeKind_Block,
    NodeKind_Int,
    NodeKind_Var,
    NodeKind_Decl,
    NodeKind_Assign,
    NodeKind_Add,
    NodeKind_Sub,
    NodeKind_Mul,
    NodeKind_Div,
    NodeKind_Neg,
    NodeKind_COUNT
};

typedef struct Node Node;
struct Node
{
    NodeKind kind;
    i64 value;  // Int
    Str8 name;  // Var, Decl, Assign; borrows the source
    i64 *slot;  // filled by check_resolve, not by the parser
    Node *lhs;  // Neg's operand, Decl and Assign's value, Block's first statement
    Node *rhs;  // a binary operator's right side, Block's trailing expression
    Node *next; // the next statement in a Block, otherwise 0
};

// Statements chain through `next` rather than nesting, so a block of any length
// costs the walkers a loop instead of a stack frame each.
#define NodeBlockHasValue(node) ((node)->kind == NodeKind_Block && (node)->rhs != 0)

// Parses `src` whole into a Block. On success *out_ast is the tree, pushed onto
// `arena`. On failure *out_ast is 0 and *out_err is a message, also on `arena`.
b32 parse_line(Arena *arena, Str8 src, Node **out_ast, Str8 *out_err);

#endif // PARSE_H
