// parse.h - tokens to an expression tree.
//
// The grammar, tightest binding last:
//
//     expr    := term (('+' | '-') term)*
//     term    := unary (('*' | '/') unary)*
//     unary   := '-' unary | primary
//     primary := INT | '(' expr ')'

#ifndef PARSE_H
#define PARSE_H

#include "../base/base.h"

typedef enum NodeKind NodeKind;
enum NodeKind
{
    NodeKind_Int,
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
    Node *lhs; // the operand for NodeKind_Neg
    Node *rhs; // 0 for NodeKind_Int and NodeKind_Neg
};

// Parses `src` whole. On success *out_ast is the tree, pushed onto `arena`. On
// failure *out_ast is 0 and *out_err is a message, also on `arena`.
b32 parse_expr(Arena *arena, Str8 src, Node **out_ast, Str8 *out_err);

#endif // PARSE_H
