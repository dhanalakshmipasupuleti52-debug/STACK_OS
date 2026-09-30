#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include "../include/jobs.h"

static Job job_table[MAX_JOBS];
static int next_job_id = 1;
static pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;

volatile sig_atomic_t g_sigchld_flag = 0;

void jobs_init(void) {
    memset(job_table, 0, sizeof(job_table));
}

int job_add(pid_t pgid, pid_t *pids, int npids, const char *cmdline, int background) {
    pthread_mutex_lock(&job_mutex);

    int slot = -1;

    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_table[i].in_use) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        pthread_mutex_unlock(&job_mutex);
        return -1;
    }

    Job *j = &job_table[slot];

    j->id = next_job_id++;
    j->pgid = pgid;
    j->npids = npids > MAX_JOB_PIDS ? MAX_JOB_PIDS : npids;

    for (int i = 0; i < j->npids; i++)
        j->pids[i] = pids[i];

    j->command = strdup(cmdline);
    j->state = JOB_RUNNING;
    j->background = background;
    j->in_use = 1;

    int id = j->id;

    pthread_mutex_unlock(&job_mutex);

    return id;
}

Job *job_by_id(int id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_table[i].in_use && job_table[i].id == id)
            return &job_table[i];
    }

    return NULL;
}

Job *job_by_pgid(pid_t pgid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_table[i].in_use && job_table[i].pgid == pgid)
            return &job_table[i];
    }

    return NULL;
}

void job_lock(void) {
    pthread_mutex_lock(&job_mutex);
}

void job_unlock(void) {
    pthread_mutex_unlock(&job_mutex);
}

void job_set_state(int id, JobState state) {
    pthread_mutex_lock(&job_mutex);

    Job *j = job_by_id(id);

    if (j)
        j->state = state;

    pthread_mutex_unlock(&job_mutex);
}

void job_remove(int id) {
    pthread_mutex_lock(&job_mutex);

    Job *j = job_by_id(id);

    if (j) {
        free(j->command);
        memset(j, 0, sizeof(Job));
    }

    pthread_mutex_unlock(&job_mutex);
}

void job_print_table(void) {
    pthread_mutex_lock(&job_mutex);

    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_table[i].in_use)
            continue;

        Job *j = &job_table[i];

        const char *state =
            j->state == JOB_RUNNING ? "Running" :
            j->state == JOB_STOPPED ? "Stopped" :
            "Done";

        printf("[%d]%s  %-8s %s\n",
               j->id,
               j->background ? "+" : " ",
               state,
               j->command);
    }

    pthread_mutex_unlock(&job_mutex);
}

void job_reap_check(void) {
    pthread_mutex_lock(&job_mutex);

    for (int i = 0; i < MAX_JOBS; i++) {

        if (!job_table[i].in_use)
            continue;

        Job *j = &job_table[i];

        /*
         * Foreground jobs are handled by executor.c.
         * The monitor only watches background jobs.
         */
        if (!j->background)
            continue;

        if (j->state == JOB_DONE)
            continue;

        int all_done = 1;

        for (int p = 0; p < j->npids; p++) {

            if (j->pids[p] == 0)
                continue;

            int status;

            pid_t r = waitpid(
                j->pids[p],
                &status,
                WNOHANG | WUNTRACED | WCONTINUED
            );

            if (r == 0) {
                all_done = 0;
            }
            else if (r == j->pids[p]) {

                if (WIFSTOPPED(status)) {
                    j->state = JOB_STOPPED;
                    all_done = 0;
                }
                else if (WIFCONTINUED(status)) {
                    j->state = JOB_RUNNING;
                    all_done = 0;
                }
                else {
                    j->pids[p] = 0;
                }
            }
        }

        if (all_done && j->state != JOB_STOPPED) {

            j->state = JOB_DONE;

            if (j->background) {
                printf(
                    "\n[%d]+  Done                    %s\n",
                    j->id,
                    j->command
                );
            }
        }
    }

    pthread_mutex_unlock(&job_mutex);
}
