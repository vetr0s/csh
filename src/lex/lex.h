// lex.h - source text to tokens.
//
// The lexer holds a borrowed Str8 and a cursor. It allocates nothing, so a
// token is only meaningful while the source it came from is alive.

#ifndef LEX_H
#define LEX_H

#include "../base/base.h"

typedef enum TokenKind TokenKind;
enum TokenKind
{
    TokenKind_EOF,
    TokenKind_Int,
    TokenKind_Ident,
    TokenKind_KeywordI64,
    TokenKind_Plus,
    TokenKind_Minus,
    TokenKind_Star,
    TokenKind_Slash,
    TokenKind_Assign,
    TokenKind_Semi,
    TokenKind_LParen,
    TokenKind_RParen,
    TokenKind_Error,    // a byte that starts no token
    TokenKind_Overflow, // digits that do not fit an i64
    TokenKind_COUNT
};

typedef struct Token Token;
struct Token
{
    TokenKind kind;
    i64 value;  // TokenKind_Int only
    Str8 text;  // TokenKind_Ident only; borrows the source
    u64 offset; // byte offset into the source, for error reporting
};

typedef struct Lexer Lexer;
struct Lexer
{
    Str8 src;
    u64 pos;
};

void lex_init(Lexer *lex, Str8 src);

// Always advances past what it returns, so an Error token cannot loop the
// caller. At the end of the source it returns EOF forever.
Token lex_next(Lexer *lex);

char *lex_token_kind_name(TokenKind kind);

#endif // LEX_H
