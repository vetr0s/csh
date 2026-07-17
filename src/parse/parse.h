// parse.h - tokens to a tree.
//
// The grammar, tightest binding last:
//
//     line    := decl | expr
//     decl    := 'i64' IDENT '=' expr ';'
//     expr    := term (('+' | '-') term)*
//     term    := unary (('*' | '/') unary)*
//     unary   := '-' unary | primary
//     primary := INT | IDENT | '(' expr ')'
//
// A bare `x = 1` is not assignment yet. Redeclaring is how a value changes.

#ifndef PARSE_H
#define PARSE_H

#include "../base/base.h"

typedef enum NodeKind NodeKind;
enum NodeKind
{
    NodeKind_Int,
    NodeKind_Var,
    NodeKind_Decl,
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
    i64 value; // NodeKind_Int only
    Str8 name; // NodeKind_Var and NodeKind_Decl; borrows the source
    i64 *slot; // filled by check_resolve, not by the parser
    Node *lhs; // the operand for NodeKind_Neg, the initialiser for NodeKind_Decl
    Node *rhs; // 0 for everything but a binary operator
};

// Parses `src` whole. On success *out_ast is the tree, pushed onto `arena`. On
// failure *out_ast is 0 and *out_err is a message, also on `arena`.
b32 parse_line(Arena *arena, Str8 src, Node **out_ast, Str8 *out_err);

#endif // PARSE_H
