#ifndef PARSER_H
#define PARSER_H
/*
 * Week 3 - The Parser (Part B: pipeline AST)
 * Turns a TokenList into a Pipeline: an arbitrary-length chain of
 * Commands connected by pipes, each with optional redirections.
 */
#include "lexer.h"

typedef struct {
    char **argv;      /* NULL-terminated */
    int    argc;
    char  *in_file;   /* < file, or NULL       */
    char  *out_file;  /* > or >> file, or NULL  */
    int    append;    /* 1 if >>, 0 if >        */
    char  *err_file;  /* 2> file, or NULL       */
} Command;

typedef struct {
    Command *cmds;
    int      ncmds;
    int      background;  /* trailing & */
} Pipeline;

/* Returns 1 on success, 0 on a syntax error (message printed to stderr) */
int  parse_tokens(TokenList *tl, Pipeline *out);
void pipeline_free(Pipeline *pl);

#endif
