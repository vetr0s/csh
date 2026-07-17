// parse.h - tokens to a tree.
//
// The grammar, tightest binding last:
//
//     line    := decl | expr
//     decl    := 'i64' IDENT '=' expr ';'
//     expr    := IDENT '=' expr | addsub
//     addsub  := term (('+' | '-') term)*
//     term    := unary (('*' | '/') unary)*
//     unary   := '-' unary | primary
//     primary := INT | IDENT | '(' expr ')'
//
// Assignment is an expression, as in C, so it is the lowest precedence, binds
// to the right, and has the assigned value. Only a name may sit left of '='.

#ifndef PARSE_H
#define PARSE_H

#include "../base/base.h"

typedef enum NodeKind NodeKind;
enum NodeKind
{
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
    i64 value; // NodeKind_Int only
    Str8 name; // Var, Decl, and Assign; borrows the source
    i64 *slot; // filled by check_resolve, not by the parser
    Node *lhs; // the operand of Neg, the value of Decl and Assign
    Node *rhs; // 0 for everything but a binary operator
};

// Parses `src` whole. On success *out_ast is the tree, pushed onto `arena`. On
// failure *out_ast is 0 and *out_err is a message, also on `arena`.
b32 parse_line(Arena *arena, Str8 src, Node **out_ast, Str8 *out_err);

#endif // PARSE_H
