#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include "../include/monitor.h"
#include "../include/jobs.h"

static pthread_t     monitor_thread;
static volatile int  monitor_running = 0;

static void *monitor_loop(void *arg) {
    (void)arg;
    while (monitor_running) {
        /* The SIGCHLD handler (Week 6) only sets a sig_atomic_t flag -
         * all real work (locking, waitpid, printing) happens here in
         * normal thread context, never inside the signal handler. */
        if (g_sigchld_flag) {
            g_sigchld_flag = 0;
            job_reap_check();
        }
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 50 * 1000 * 1000 }; /* 50ms */
        nanosleep(&ts, NULL);
    }
    return NULL;
}

void monitor_start(void) {
    monitor_running = 1;
    pthread_create(&monitor_thread, NULL, monitor_loop, NULL);
}

void monitor_stop(void) {
    monitor_running = 0;
    pthread_join(monitor_thread, NULL);
}
