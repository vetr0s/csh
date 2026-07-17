// lex.c - see lex.h.

#include "lex.h"

void lex_init(Lexer *lex, Str8 src)
{
    lex->src = src;
    lex->pos = 0;
}

static b32 lex_is_digit_(u8 c)
{
    return c >= '0' && c <= '9';
}

static b32 lex_is_space_(u8 c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static b32 lex_is_ident_start_(u8 c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static b32 lex_is_ident_cont_(u8 c)
{
    return lex_is_ident_start_(c) || lex_is_digit_(c);
}

// The token borrows the source, so it dies with the line it came from. Anything
// that outlives the line copies the name first.
static Token lex_ident_(Lexer *lex)
{
    Token token  = {0};
    token.offset = lex->pos;

    u64 start = lex->pos;
    while (lex->pos < lex->src.size && lex_is_ident_cont_(lex->src.str[lex->pos]))
    {
        lex->pos += 1;
    }
    token.text = str8(lex->src.str + start, lex->pos - start);
    token.kind = str8_equal(token.text, Str8Lit("i64")) ? TokenKind_KeywordI64 : TokenKind_Ident;
    return token;
}

// Reports overflow rather than wrapping, so `999...9` is a diagnostic instead
// of a silently wrong answer.
static Token lex_int_(Lexer *lex)
{
    Token token  = {0};
    token.offset = lex->pos;
    token.kind   = TokenKind_Int;

    u64 value = 0;
    while (lex->pos < lex->src.size && lex_is_digit_(lex->src.str[lex->pos]))
    {
        u64 digit = (u64)(lex->src.str[lex->pos] - '0');
        if (value > ((u64)INT64_MAX - digit) / 10)
        {
            token.kind = TokenKind_Overflow;
        }
        else
        {
            value = value * 10 + digit;
        }
        lex->pos += 1;
    }

    token.value = (i64)value;
    return token;
}

Token lex_next(Lexer *lex)
{
    while (lex->pos < lex->src.size && lex_is_space_(lex->src.str[lex->pos]))
    {
        lex->pos += 1;
    }

    Token token  = {0};
    token.offset = lex->pos;

    if (lex->pos >= lex->src.size)
    {
        token.kind = TokenKind_EOF;
        return token;
    }

    u8 c = lex->src.str[lex->pos];
    if (lex_is_digit_(c))
    {
        return lex_int_(lex);
    }
    if (lex_is_ident_start_(c))
    {
        return lex_ident_(lex);
    }

    switch (c)
    {
    case '=':
        token.kind = TokenKind_Assign;
        break;
    case ';':
        token.kind = TokenKind_Semi;
        break;
    case '+':
        token.kind = TokenKind_Plus;
        break;
    case '-':
        token.kind = TokenKind_Minus;
        break;
    case '*':
        token.kind = TokenKind_Star;
        break;
    case '/':
        token.kind = TokenKind_Slash;
        break;
    case '(':
        token.kind = TokenKind_LParen;
        break;
    case ')':
        token.kind = TokenKind_RParen;
        break;
    default:
        token.kind = TokenKind_Error;
        break;
    }
    lex->pos += 1;
    return token;
}

// Left unsized so the StaticAssert below fails if a TokenKind is added without
// a name. A sized array would quietly hold a null here.
static char *lex_kind_names_[] = {
    "end of input",
    "an integer",
    "a name",
    "'i64'",
    "'+'",
    "'-'",
    "'*'",
    "'/'",
    "'='",
    "';'",
    "'('",
    "')'",
    "a bad character",
    "an integer too large for i64",
};
StaticAssert(ArrayCount(lex_kind_names_) == TokenKind_COUNT, lex_kind_names_complete);

char *lex_token_kind_name(TokenKind kind)
{
    if ((u32)kind >= TokenKind_COUNT)
    {
        return "an unknown token";
    }
    return lex_kind_names_[kind];
}
