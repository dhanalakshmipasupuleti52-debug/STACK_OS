#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/vec.h"

void str_init(String *s) {
    s->cap  = 32;
    s->len  = 0;
    s->data = malloc(s->cap);
    s->data[0] = '\0';
}

void str_free(String *s) {
    free(s->data);
    s->data = NULL;
    s->len = s->cap = 0;
}

static void str_grow(String *s, size_t needed) {
    if (needed < s->cap) return;
    while (s->cap <= needed) s->cap *= 2;
    s->data = realloc(s->data, s->cap);
}

void str_push(String *s, char c) {
    str_grow(s, s->len + 1);
    s->data[s->len++] = c;
    s->data[s->len]   = '\0';
}

void str_append(String *s, const char *text) {
    size_t n = strlen(text);
    str_grow(s, s->len + n);
    memcpy(s->data + s->len, text, n);
    s->len += n;
    s->data[s->len] = '\0';
}

void str_clear(String *s) {
    s->len = 0;
    if (s->data) s->data[0] = '\0';
}

void vec_init(Vec *v) {
    v->cap   = 8;
    v->count = 0;
    v->items = malloc(v->cap * sizeof(char *));
}

void vec_push(Vec *v, char *item) {
    if (v->count == v->cap) {
        v->cap *= 2;
        v->items = realloc(v->items, v->cap * sizeof(char *));
    }
    v->items[v->count++] = item;
}

void vec_free(Vec *v) {
    for (size_t i = 0; i < v->count; i++) free(v->items[i]);
    free(v->items);
    v->items = NULL;
    v->count = v->cap = 0;
}

char **vec_to_argv(Vec *v) {
    /* caller owns the returned array (not the strings) */
    char **argv = malloc((v->count + 1) * sizeof(char *));
    for (size_t i = 0; i < v->count; i++) argv[i] = v->items[i];
    argv[v->count] = NULL;
    return argv;
}
