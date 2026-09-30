#ifndef BUILTINS_H
#define BUILTINS_H
/*
 * Week 5 - fork / exec / wait in Anger
 * Built-ins run *inside* the shell process (no fork) because they
 * must mutate shell state: cd changes the shell's own cwd, exit
 * ends the shell's own event loop, fg/bg touch the job table.
 */
#include "parser.h"

int is_builtin(const char *name);
/* returns the exit status to report via $?, and sets *should_exit
 * to 1 if the built-in was `exit` */
int run_builtin(Command *cmd, int *should_exit);

#endif
