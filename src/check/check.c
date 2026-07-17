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
    // Iterating rather than recursing, so a file of ten thousand statements
    // costs one stack frame instead of ten thousand.
    case NodeKind_Block:
    {
        for (Node *stmt = node->lhs; stmt != 0; stmt = stmt->next)
        {
            check_node_(checker, stmt);
            if (checker->failed)
            {
                return;
            }
        }
        if (node->rhs != 0)
        {
            check_node_(checker, node->rhs);
        }
    }
    break;

    case NodeKind_Int:
        break;

    case NodeKind_Var:
    {
        SymRef ref = sym_lookup(checker->syms, node->name);
        if (ref.slot == 0)
        {
            checker_fail_(checker,
                          str8_concat(checker->arena, Str8Lit("undefined name: "), node->name));
            return;
        }
        node->slot = ref.slot;
    }
    break;

    case NodeKind_Assign:
    {
        // The target resolves first, so `nope = 1` reports nope rather than
        // whatever the value happens to mention.
        SymRef ref = sym_lookup(checker->syms, node->name);
        if (ref.slot == 0)
        {
            checker_fail_(checker,
                          str8_concat(checker->arena, Str8Lit("undefined name: "), node->name));
            return;
        }
        if (ref.is_const)
        {
            checker_fail_(
                checker,
                str8_concat(checker->arena, Str8Lit("cannot assign to the constant "), node->name));
            return;
        }
        node->slot = ref.slot;
        check_node_(checker, node->lhs);
    }
    break;

    case NodeKind_Decl:
    {
        // The initialiser resolves first, so `x := x + 1;` reads the old x and
        // `y := y;` fails instead of reading a slot it just made.
        check_node_(checker, node->lhs);
        if (checker->failed)
        {
            return;
        }

        // The parser passed the type through as text. This is the only place
        // that knows i64 is a type and not just a name someone typed.
        if (node->type_name.size > 0 && !str8_equal(node->type_name, Str8Lit("i64")))
        {
            checker_fail_(checker,
                          str8_concat(checker->arena, Str8Lit("unknown type: "), node->type_name));
            return;
        }

        SymRef ref = sym_declare(checker->syms, node->name, node->is_const);
        node->slot = ref.slot;
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
