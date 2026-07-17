// parse.c - see parse.h.

#include "parse.h"

#include "../lex/lex.h"

typedef struct Parser Parser;
struct Parser
{
    Arena *arena;
    Lexer lexer;
    Token token; // one token of lookahead
    Str8 err;
    b32 failed;
};

// Once failed, every rule below unwinds without reporting again, so the first
// message survives instead of the last.
static void parser_fail_(Parser *parser, Str8 msg)
{
    if (!parser->failed)
    {
        parser->failed = 1;
        parser->err    = msg;
    }
}

static void parser_fail_expected_(Parser *parser, Str8 expected)
{
    Str8 msg = str8_concat(parser->arena, Str8Lit("expected "), expected);
    msg      = str8_concat(parser->arena, msg, Str8Lit(", found "));
    msg = str8_concat(parser->arena, msg, str8_from_cstr(lex_token_kind_name(parser->token.kind)));
    parser_fail_(parser, msg);
}

static void parser_advance_(Parser *parser)
{
    parser->token = lex_next(&parser->lexer);
}

static Node *parser_node_(Parser *parser, NodeKind kind)
{
    Node *node = PushStruct(parser->arena, Node);
    if (node == 0)
    {
        parser_fail_(parser, Str8Lit("out of memory"));
        return 0;
    }
    node->kind = kind;
    return node;
}

static Node *parser_expr_(Parser *parser);

static Node *parser_primary_(Parser *parser)
{
    if (parser->token.kind == TokenKind_Int)
    {
        Node *node = parser_node_(parser, NodeKind_Int);
        if (node == 0)
        {
            return 0;
        }
        node->value = parser->token.value;
        parser_advance_(parser);
        return node;
    }

    if (parser->token.kind == TokenKind_Ident)
    {
        Node *node = parser_node_(parser, NodeKind_Var);
        if (node == 0)
        {
            return 0;
        }
        node->name = parser->token.text;
        parser_advance_(parser);
        return node;
    }

    if (parser->token.kind == TokenKind_LParen)
    {
        parser_advance_(parser);
        Node *inner = parser_expr_(parser);
        if (parser->failed)
        {
            return 0;
        }
        if (parser->token.kind != TokenKind_RParen)
        {
            parser_fail_expected_(parser, Str8Lit("')'"));
            return 0;
        }
        parser_advance_(parser);
        return inner;
    }

    parser_fail_expected_(parser, Str8Lit("an expression"));
    return 0;
}

static Node *parser_unary_(Parser *parser)
{
    if (parser->token.kind == TokenKind_Minus)
    {
        parser_advance_(parser);
        Node *operand = parser_unary_(parser);
        if (parser->failed)
        {
            return 0;
        }
        Node *node = parser_node_(parser, NodeKind_Neg);
        if (node == 0)
        {
            return 0;
        }
        node->lhs = operand;
        return node;
    }
    return parser_primary_(parser);
}

static Node *parser_term_(Parser *parser)
{
    Node *lhs = parser_unary_(parser);
    while (!parser->failed &&
           (parser->token.kind == TokenKind_Star || parser->token.kind == TokenKind_Slash))
    {
        NodeKind kind = (parser->token.kind == TokenKind_Star) ? NodeKind_Mul : NodeKind_Div;
        parser_advance_(parser);
        Node *rhs = parser_unary_(parser);
        if (parser->failed)
        {
            return 0;
        }
        Node *node = parser_node_(parser, kind);
        if (node == 0)
        {
            return 0;
        }
        node->lhs = lhs;
        node->rhs = rhs;
        lhs       = node;
    }
    return lhs;
}

static Node *parser_addsub_(Parser *parser)
{
    Node *lhs = parser_term_(parser);
    while (!parser->failed &&
           (parser->token.kind == TokenKind_Plus || parser->token.kind == TokenKind_Minus))
    {
        NodeKind kind = (parser->token.kind == TokenKind_Plus) ? NodeKind_Add : NodeKind_Sub;
        parser_advance_(parser);
        Node *rhs = parser_term_(parser);
        if (parser->failed)
        {
            return 0;
        }
        Node *node = parser_node_(parser, kind);
        if (node == 0)
        {
            return 0;
        }
        node->lhs = lhs;
        node->rhs = rhs;
        lhs       = node;
    }
    return lhs;
}

// The target is parsed as an ordinary operand and then checked, rather than
// peeked at, because deciding on '=' needs the whole left side in hand first.
static Node *parser_expr_(Parser *parser)
{
    Node *lhs = parser_addsub_(parser);
    if (parser->failed || parser->token.kind != TokenKind_Assign)
    {
        return lhs;
    }

    if (lhs->kind != NodeKind_Var)
    {
        parser_fail_(parser, Str8Lit("only a name can go left of '='"));
        return 0;
    }
    Str8 name = lhs->name;
    parser_advance_(parser);

    // Recursing here rather than looping is what makes `a = b = 1` bind right.
    Node *value = parser_expr_(parser);
    if (parser->failed)
    {
        return 0;
    }

    Node *node = parser_node_(parser, NodeKind_Assign);
    if (node == 0)
    {
        return 0;
    }
    node->name = name;
    node->lhs  = value;
    return node;
}

// decl := 'i64' IDENT '=' expr ';'
static Node *parser_decl_(Parser *parser)
{
    parser_advance_(parser); // 'i64'

    if (parser->token.kind != TokenKind_Ident)
    {
        parser_fail_expected_(parser, Str8Lit("a name"));
        return 0;
    }
    Str8 name = parser->token.text;
    parser_advance_(parser);

    if (parser->token.kind != TokenKind_Assign)
    {
        parser_fail_expected_(parser, Str8Lit("'='"));
        return 0;
    }
    parser_advance_(parser);

    Node *init = parser_expr_(parser);
    if (parser->failed)
    {
        return 0;
    }

    if (parser->token.kind != TokenKind_Semi)
    {
        parser_fail_expected_(parser, Str8Lit("';'"));
        return 0;
    }
    parser_advance_(parser);

    Node *node = parser_node_(parser, NodeKind_Decl);
    if (node == 0)
    {
        return 0;
    }
    node->name = name;
    node->lhs  = init;
    return node;
}

b32 parse_line(Arena *arena, Str8 src, Node **out_ast, Str8 *out_err)
{
    *out_ast = 0;
    *out_err = Str8Lit("");

    Parser parser = {0};
    parser.arena  = arena;
    lex_init(&parser.lexer, src);
    parser_advance_(&parser);

    Node *ast =
        (parser.token.kind == TokenKind_KeywordI64) ? parser_decl_(&parser) : parser_expr_(&parser);

    // Trailing junk is an error, otherwise `1 2` would quietly evaluate to 1.
    if (!parser.failed && parser.token.kind != TokenKind_EOF)
    {
        parser_fail_expected_(&parser, Str8Lit("end of input"));
    }

    if (parser.failed)
    {
        *out_err = parser.err;
        return 0;
    }

    *out_ast = ast;
    return 1;
}
