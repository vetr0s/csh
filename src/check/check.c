// check.c - see check.h.

#include "check.h"

typedef struct Checker Checker;
struct Checker
{
    Arena *arena;
    SymTable *syms;
    Str8 err;
    b32 failed;
};

static void checker_fail_(Checker *checker, Str8 msg)
{
    if (!checker->failed)
    {
        checker->failed = 1;
        checker->err    = msg;
    }
}

static void check_node_(Checker *checker, Node *node)
{
    if (checker->failed)
    {
        return;
    }

    switch (node->kind)
    {
    case NodeKind_Int:
        break;

    case NodeKind_Var:
    {
        node->slot = sym_lookup(checker->syms, node->name);
        if (node->slot == 0)
        {
            checker_fail_(checker,
                          str8_concat(checker->arena, Str8Lit("undefined name: "), node->name));
        }
    }
    break;

    case NodeKind_Decl:
    {
        // The initialiser resolves first, so `i64 x = x + 1;` reads the old x
        // and `i64 y = y;` fails instead of reading a slot it just made.
        check_node_(checker, node->lhs);
        if (checker->failed)
        {
            return;
        }
        node->slot = sym_declare(checker->syms, node->name);
        if (node->slot == 0)
        {
            checker_fail_(checker, Str8Lit("out of memory"));
        }
    }
    break;

    case NodeKind_Neg:
    case NodeKind_Add:
    case NodeKind_Sub:
    case NodeKind_Mul:
    case NodeKind_Div:
    {
        check_node_(checker, node->lhs);
        if (node->rhs != 0)
        {
            check_node_(checker, node->rhs);
        }
    }
    break;

    default:
        Assert(!"check_node_: unhandled NodeKind");
        break;
    }
}

b32 check_resolve(Arena *arena, SymTable *syms, Node *ast, Str8 *out_err)
{
    *out_err = Str8Lit("");

    Checker checker = {0};
    checker.arena   = arena;
    checker.syms    = syms;

    check_node_(&checker, ast);

    if (checker.failed)
    {
        *out_err = checker.err;
        return 0;
    }
    return 1;
}
