#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "../include/builtins.h"
#include "../include/jobs.h"
#include "../include/signals.h"
#include "../include/memstat.h"

static const char *BUILTIN_NAMES[] = { "cd", "exit", "help", "memstat", "jobs", "fg", "bg", NULL };

int is_builtin(const char *name) {
    for (int i = 0; BUILTIN_NAMES[i]; i++)
        if (strcmp(name, BUILTIN_NAMES[i]) == 0) return 1;
    return 0;
}

static void print_help(void) {
    printf(
        "EduShell - a mini OS shell, built one week at a time\n"
        "  cd [dir]        change directory (default: $HOME)\n"
        "  jobs             list background/stopped jobs\n"
        "  fg %%N            bring job N to the foreground\n"
        "  bg %%N            resume job N in the background\n"
        "  memstat          show this shell's VmRSS / VmSize (Week 8)\n"
        "  exit [code]      leave EduShell\n"
        "  <cmd> [args] [< in] [> out] [>> out] [2> err] [| cmd2 ...] [&]\n");
}

static int job_id_from_arg(const char *arg) {
    if (arg && arg[0] == '%') return atoi(arg + 1);
    return atoi(arg ? arg : "0");
}

/* Waits (blocking) for every pid in a job that is still alive, used by
 * `fg`. Mirrors the foreground wait loop in executor.c so a resumed
 * job behaves identically to one that was foreground from the start. */
static void wait_for_job_foreground(int id) {
    job_lock();
    Job *j = job_by_id(id);
    pid_t pgid = j ? j->pgid : 0;
    pid_t pids[MAX_JOB_PIDS];
    int npids = j ? j->npids : 0;
    for (int i = 0; i < npids; i++) pids[i] = j->pids[i];
    job_unlock();
    if (!pgid) { printf("bg/fg: no such job\n"); return; }

    shell_set_foreground_pgid(pgid);
    int stopped = 0;
    for (int i = 0; i < npids; i++) {
        if (pids[i] == 0) continue;
        int status;
        if (waitpid(pids[i], &status, WUNTRACED) > 0 && WIFSTOPPED(status)) stopped = 1;
    }
    shell_set_foreground_pgid(0);

    if (stopped) {
        job_set_state(id, JOB_STOPPED);
        printf("[%d]+  Stopped\n", id);
    } else {
        job_remove(id);
    }
}

int run_builtin(Command *cmd, int *should_exit) {
    *should_exit = 0;
    const char *name = cmd->argv[0];

    if (strcmp(name, "exit") == 0) {
        *should_exit = 1;
        return cmd->argc > 1 ? atoi(cmd->argv[1]) : 0;
    }
    if (strcmp(name, "help") == 0) { print_help(); return 0; }
    if (strcmp(name, "memstat") == 0) { memstat_print(); return 0; }
    if (strcmp(name, "jobs") == 0) { job_print_table(); return 0; }

    if (strcmp(name, "cd") == 0) {
        const char *dest = cmd->argc > 1 ? cmd->argv[1] : getenv("HOME");
        if (!dest || chdir(dest) != 0) {
            fprintf(stderr, "cd: %s: No such directory\n", dest ? dest : "");
            return 1;
        }
        return 0;
    }

    if (strcmp(name, "bg") == 0) {
        if (cmd->argc < 2) { fprintf(stderr, "bg: usage: bg %%job_id\n"); return 1; }
        int id = job_id_from_arg(cmd->argv[1]);
        job_lock();
        Job *j = job_by_id(id);
        pid_t pgid = j ? j->pgid : 0;
        if (j) j->state = JOB_RUNNING;
        job_unlock();
        if (!pgid) { fprintf(stderr, "bg: no such job\n"); return 1; }
        killpg(pgid, SIGCONT);
        printf("[%d]+  %s &\n", id, "continued");
        return 0;
    }

    if (strcmp(name, "fg") == 0) {
        if (cmd->argc < 2) { fprintf(stderr, "fg: usage: fg %%job_id\n"); return 1; }
        int id = job_id_from_arg(cmd->argv[1]);
        job_lock();
        Job *j = job_by_id(id);
        pid_t pgid = j ? j->pgid : 0;
        job_unlock();
        if (!pgid) { fprintf(stderr, "fg: no such job\n"); return 1; }
        killpg(pgid, SIGCONT);
        wait_for_job_foreground(id);
        return 0;
    }

    return 0;
}
