#ifndef VEC_H
#define VEC_H
/*
 * Week 2 - The C Toolchain & Memory Model
 * Minimal dynamic string and dynamic array (vector) types.
 * Every EduShell module builds on these two primitives instead of
 * fixed-size char buffers, so growth is malloc/realloc-driven and
 * valgrind-clean (see Week 8).
 */
#include <stddef.h>

/* ---- Dynamic string (grow-only char buffer) ---- */
typedef struct {
    char   *data;
    size_t  len;
    size_t  cap;
} String;

void   str_init(String *s);
void   str_free(String *s);
void   str_push(String *s, char c);
void   str_append(String *s, const char *text);
void   str_clear(String *s);

/* ---- Dynamic array of owned char* (used for argv / token lists) ---- */
typedef struct {
    char  **items;
    size_t  count;
    size_t  cap;
} Vec;

void   vec_init(Vec *v);
void   vec_push(Vec *v, char *item);   /* takes ownership of item */
void   vec_free(Vec *v);               /* frees every item + the array */
char **vec_to_argv(Vec *v);            /* returns a NULL-terminated char** view */

#endif
