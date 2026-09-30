#ifndef JOBS_H
#define JOBS_H
/*
 * Week 11 - Concurrency II: Deadlock & Jobs
 * A mutex-protected job table shared between the main REPL thread
 * (which adds jobs and services `jobs` / `fg` / `bg`) and the
 * background monitor thread from Week 10 (which reaps children and
 * flips job state). All access goes through these functions so the
 * lock is never held across a blocking syscall - the classic
 * deadlock trap this module is designed to teach.
 */
#include <sys/types.h>
#include <signal.h>

#define MAX_JOBS      64
#define MAX_JOB_PIDS   8

typedef enum { JOB_RUNNING, JOB_STOPPED, JOB_DONE } JobState;

typedef struct {
    int      id;
    pid_t    pgid;
    pid_t    pids[MAX_JOB_PIDS];
    int      npids;
    char    *command;      /* owned copy, for `jobs` display          */
    JobState state;
    int      background;   /* launched with trailing &                */
    int      in_use;
} Job;

void  jobs_init(void);
int   job_add(pid_t pgid, pid_t *pids, int npids, const char *cmdline, int background);
Job  *job_by_id(int id);              /* caller must hold job_lock()   */
Job  *job_by_pgid(pid_t pgid);        /* caller must hold job_lock()   */
void  job_remove(int id);
void  job_print_table(void);          /* implements the `jobs` builtin */
void  job_lock(void);
void  job_unlock(void);
void  job_set_state(int id, JobState state);

/* called only by the Week 10 monitor thread */
void  job_reap_check(void);

/* async-signal-safe flag set by the SIGCHLD handler (Week 6),
 * consumed by the monitor thread (Week 10) */
extern volatile sig_atomic_t g_sigchld_flag;

#endif
