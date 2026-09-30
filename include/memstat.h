#ifndef MEMSTAT_H
#define MEMSTAT_H
/*
 * Week 8 - Virtual Memory
 * `memstat` is a built-in that reads this process's own virtual
 * memory footprint straight out of /proc, the same numbers `top`
 * and `ps` compute from. Used alongside `valgrind --leak-check=full`
 * to keep every earlier module leak-free.
 */

void memstat_print(void);

#endif
