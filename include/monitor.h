#ifndef MONITOR_H
#define MONITOR_H
/*
 * Week 10 - Concurrency I: Mutual Exclusion
 * A background pthread that polls for the SIGCHLD flag and reaps
 * children into the mutex-protected job table (jobs.c), so the
 * main REPL thread never blocks waiting on children it isn't
 * currently in the foreground for.
 */

void monitor_start(void);   /* spawns the daemon thread              */
void monitor_stop(void);    /* signals it to exit and joins it       */

#endif
