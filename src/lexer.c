#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/lexer.h"

static void tl_init(TokenList *tl) {
    tl->cap = 16;
    tl->count = 0;
    tl->items = malloc(tl->cap * sizeof(Token));
}

static void tl_push(TokenList *tl, TokenType type, char *text) {
    if (tl->count == tl->cap) {
        tl->cap *= 2;
        tl->items = realloc(tl->items, tl->cap * sizeof(Token));
    }
    tl->items[tl->count].type = type;
    tl->items[tl->count].text = text;
    tl->count++;
}

void tokenlist_free(TokenList *tl) {
    for (size_t i = 0; i < tl->count; i++)
        if (tl->items[i].text) free(tl->items[i].text);
    free(tl->items);
    tl->items = NULL;
    tl->count = tl->cap = 0;
}

/* reads one WORD token starting at *p, honoring '...' and "..." quoting */
static char *read_word(const char **p) {
    String s; str_init(&s);
    while (**p && !isspace((unsigned char)**p) &&
           **p != '|' && **p != '<' && **p != '>' && **p != '&') {
        char c = **p;
        if (c == '\'' || c == '"') {
            char quote = c;
            (*p)++;
            while (**p && **p != quote) { str_push(&s, **p); (*p)++; }
            if (**p == quote) (*p)++;
        } else {
            str_push(&s, c);
            (*p)++;
        }
    }
    char *out = strdup(s.data);
    str_free(&s);
    return out;
}

void tokenize(const char *line, TokenList *out) {
    tl_init(out);
    const char *p = line;
    while (*p) {
        if (isspace((unsigned char)*p)) { p++; continue; }
        if (*p == '|') { tl_push(out, TOK_PIPE, NULL); p++; continue; }
        if (*p == '&') { tl_push(out, TOK_AMP, NULL);  p++; continue; }
        if (*p == '<') { tl_push(out, TOK_REDIR_IN, NULL); p++; continue; }
        if (*p == '>') {
            if (*(p + 1) == '>') { tl_push(out, TOK_REDIR_APP, NULL); p += 2; }
            else                 { tl_push(out, TOK_REDIR_OUT, NULL); p += 1; }
            continue;
        }
        if (*p == '2' && *(p + 1) == '>') {
            tl_push(out, TOK_REDIR_ERR, NULL); p += 2; continue;
        }
        char *word = read_word(&p);
        tl_push(out, TOK_WORD, word);
    }
    tl_push(out, TOK_EOF, NULL);
}
