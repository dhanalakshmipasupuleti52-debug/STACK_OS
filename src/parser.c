#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/parser.h"

static void cmd_init(Command *c) {
    c->argv = NULL; c->argc = 0;
    c->in_file = c->out_file = c->err_file = NULL;
    c->append = 0;
}

static void cmd_add_arg(Vec *argv, const char *word) {
    vec_push(argv, strdup(word));
}

int parse_tokens(TokenList *tl, Pipeline *out) {
    out->cmds = malloc(sizeof(Command) * 8);
    int cap = 8;
    out->ncmds = 0;
    out->background = 0;

    Vec argv; vec_init(&argv);
    Command cur; cmd_init(&cur);

    size_t i = 0;
    while (i < tl->count && tl->items[i].type != TOK_EOF) {
        Token *t = &tl->items[i];
        switch (t->type) {
        case TOK_WORD:
            cmd_add_arg(&argv, t->text);
            i++;
            break;
        case TOK_REDIR_IN:
            if (tl->items[i + 1].type != TOK_WORD) {
                fprintf(stderr, "eduShell: syntax error: expected file after '<'\n");
                return 0;
            }
            cur.in_file = strdup(tl->items[i + 1].text);
            i += 2;
            break;
        case TOK_REDIR_OUT:
        case TOK_REDIR_APP:
            if (tl->items[i + 1].type != TOK_WORD) {
                fprintf(stderr, "eduShell: syntax error: expected file after redirect\n");
                return 0;
            }
            cur.out_file = strdup(tl->items[i + 1].text);
            cur.append   = (t->type == TOK_REDIR_APP);
            i += 2;
            break;
        case TOK_REDIR_ERR:
            if (tl->items[i + 1].type != TOK_WORD) {
                fprintf(stderr, "eduShell: syntax error: expected file after '2>'\n");
                return 0;
            }
            cur.err_file = strdup(tl->items[i + 1].text);
            i += 2;
            break;
        case TOK_PIPE:
            if (argv.count == 0) {
                fprintf(stderr, "eduShell: syntax error near '|'\n");
                return 0;
            }
            cur.argc = (int)argv.count;
            cur.argv = vec_to_argv(&argv);
            if (out->ncmds == cap) { cap *= 2; out->cmds = realloc(out->cmds, sizeof(Command) * cap); }
            out->cmds[out->ncmds++] = cur;
            /* argv items were moved into cur.argv; free just the Vec array */
            free(argv.items);
            vec_init(&argv);
            cmd_init(&cur);
            i++;
            break;
        case TOK_AMP:
            out->background = 1;
            i++;
            break;
        default:
            i++;
        }
    }

    if (argv.count == 0 && out->ncmds == 0) {
        vec_free(&argv);
        out->ncmds = 0;
        return 1; /* blank line */
    }
    cur.argc = (int)argv.count;
    cur.argv = vec_to_argv(&argv);
    free(argv.items);
    if (out->ncmds == cap) { cap *= 2; out->cmds = realloc(out->cmds, sizeof(Command) * cap); }
    out->cmds[out->ncmds++] = cur;
    return 1;
}

void pipeline_free(Pipeline *pl) {
    for (int i = 0; i < pl->ncmds; i++) {
        Command *c = &pl->cmds[i];
        for (int j = 0; j < c->argc; j++) free(c->argv[j]);
        free(c->argv);
        free(c->in_file);
        free(c->out_file);
        free(c->err_file);
    }
    free(pl->cmds);
    pl->cmds = NULL;
    pl->ncmds = 0;
}
