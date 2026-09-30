#ifndef SIGNALS_H
#define SIGNALS_H
/*
 * Week 6 - Signals & Async Control
 * Ctrl-C (SIGINT) and Ctrl-Z (SIGTSTP) must not kill EduShell itself;
 * they should apply to whatever foreground pipeline is running.
 * SIGCHLD is reaped asynchronously via the Week 10 monitor thread.
 */
#include <sys/types.h>

void shell_signals_init(void);          /* call once at startup            */
void shell_set_foreground_pgid(pid_t pgid); /* pgid of the running job, or 0 */

#endif
