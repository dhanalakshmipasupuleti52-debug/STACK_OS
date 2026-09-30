#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "../include/signals.h"
#include "../include/jobs.h"

/* pgid of whatever pipeline currently owns the terminal; 0 = the shell itself */
static volatile sig_atomic_t fg_pgid = 0;

static void sigint_handler(int sig) {
    (void)sig;
    if (fg_pgid > 0) {
        killpg(fg_pgid, SIGINT);          /* forward to the foreground job */
    } else {
        write(STDOUT_FILENO, "\n", 1);    /* just redraw the prompt        */
    }
}

static void sigtstp_handler(int sig) {
    (void)sig;
    if (fg_pgid > 0) {
        killpg(fg_pgid, SIGTSTP);         /* stop the foreground job, not us */
    } else {
        write(STDOUT_FILENO, "\n", 1);
    }
}

/* Async-signal-safe: only sets a flag. All reaping happens on the
 * Week 10 monitor thread, which is the only place g_sigchld_flag is
 * cleared and job_reap_check() (which takes a mutex) is called. */
static void sigchld_handler(int sig) {
    (void)sig;
    g_sigchld_flag = 1;
}

void shell_signals_init(void) {
    struct sigaction sa_int  = { .sa_handler = sigint_handler  };
    struct sigaction sa_tstp = { .sa_handler = sigtstp_handler };
    struct sigaction sa_chld = { .sa_handler = sigchld_handler };
    sigemptyset(&sa_int.sa_mask);
    sigemptyset(&sa_tstp.sa_mask);
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;

    sigaction(SIGINT,  &sa_int,  NULL);
    sigaction(SIGTSTP, &sa_tstp, NULL);
    sigaction(SIGCHLD, &sa_chld, NULL);
    /* the shell ignores terminal-generated SIGTTOU/SIGTTIN from
     * background job group changes (classic job-control shell setup) */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}

void shell_set_foreground_pgid(pid_t pgid) {
    fg_pgid = pgid;
}
