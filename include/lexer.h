#ifndef LEXER_H
#define LEXER_H
/*
 * Week 3 - The Parser (Part A: tokenizer)
 * Splits a raw input line into shell tokens: words, "|", "<", ">",
 * ">>", "2>" and "&". Quoting with '...' and "..." is respected.
 */
#include "vec.h"

typedef enum {
    TOK_WORD,
    TOK_PIPE,       /* |  */
    TOK_REDIR_IN,   /* <  */
    TOK_REDIR_OUT,  /* >  */
    TOK_REDIR_APP,  /* >> */
    TOK_REDIR_ERR,  /* 2> */
    TOK_AMP,        /* &  */
    TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    char     *text;   /* only meaningful for TOK_WORD (owned) */
} Token;

typedef struct {
    Token  *items;
    size_t  count;
    size_t  cap;
} TokenList;

void tokenize(const char *line, TokenList *out);
void tokenlist_free(TokenList *tl);

#endif
