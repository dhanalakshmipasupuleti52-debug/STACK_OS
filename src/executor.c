#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include "../include/executor.h"
#include "../include/builtins.h"
#include "../include/jobs.h"
#include "../include/signals.h"

/* ---------------- Week 5: manual PATH search ---------------- */

char *resolve_path(const char *cmd) {
    if (strchr(cmd, '/') != NULL) {
        return access(cmd, X_OK) == 0 ? strdup(cmd) : NULL;
    }

    const char *path_env = getenv("PATH");

    if (!path_env)
        path_env = "/bin:/usr/bin";

    char *path_copy = strdup(path_env);
    if (!path_copy)
        return NULL;

    char *dir = strtok(path_copy, ":");
    char candidate[1024];

    while (dir) {
        snprintf(candidate, sizeof(candidate), "%s/%s", dir, cmd);

        if (access(candidate, X_OK) == 0) {
            free(path_copy);
            return strdup(candidate);
        }

        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}


/* ---------------- Week 9: redirection ---------------- */

static void apply_redirections(Command *c) {

    if (c->in_file) {
        int fd = open(c->in_file, O_RDONLY);

        if (fd < 0) {
            perror(c->in_file);
            _exit(1);
        }

        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (c->out_file) {
        int flags = O_WRONLY | O_CREAT;

        if (c->append)
            flags |= O_APPEND;
        else
            flags |= O_TRUNC;

        int fd = open(c->out_file, flags, 0644);

        if (fd < 0) {
            perror(c->out_file);
            _exit(1);
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    if (c->err_file) {
        int fd = open(
            c->err_file,
            O_WRONLY | O_CREAT | O_TRUNC,
            0644
        );

        if (fd < 0) {
            perror(c->err_file);
            _exit(1);
        }

        dup2(fd, STDERR_FILENO);
        close(fd);
    }
}


/* ---------------- Week 4 / 7: child execution ---------------- */

static void run_child(
    Command *c,
    int in_fd,
    int out_fd,
    pid_t pgid
) {
    /* Put child into the pipeline process group. */
    setpgid(0, pgid);

    /* Child receives normal terminal signals. */
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);

    if (in_fd != STDIN_FILENO) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }

    if (out_fd != STDOUT_FILENO) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }

    apply_redirections(c);

    /* Built-ins inside a pipeline execute in the child. */
    if (is_builtin(c->argv[0])) {
        int should_exit;

        int rc = run_builtin(c, &should_exit);

        _exit(rc);
    }

    char *exe = resolve_path(c->argv[0]);

    if (!exe) {
        fprintf(
            stderr,
            "eduShell: %s: command not found\n",
            c->argv[0]
        );

        _exit(127);
    }

    execv(exe, c->argv);

    perror("execv");

    free(exe);

    _exit(126);
}


/* ---------------- Pipeline execution ---------------- */

int execute_pipeline(
    Pipeline *pl,
    const char *raw_line
) {
    if (pl->ncmds == 0)
        return 0;


    /*
     * A single built-in must execute in the shell itself.
     * This is necessary for cd, exit, fg and bg.
     */
    if (
        pl->ncmds == 1 &&
        is_builtin(pl->cmds[0].argv[0])
    ) {
        int should_exit;

        int rc = run_builtin(
            &pl->cmds[0],
            &should_exit
        );

        if (should_exit)
            exit(rc);

        return rc;
    }


    int ncmds = pl->ncmds;

    pid_t pids[MAX_JOB_PIDS];
    int npids = 0;

    pid_t pgid = 0;

    int prev_read = STDIN_FILENO;


    /* Create all pipeline processes. */

    for (int i = 0; i < ncmds; i++) {

        int pipefd[2] = {
            STDOUT_FILENO,
            STDOUT_FILENO
        };

        if (i < ncmds - 1) {

            if (pipe(pipefd) < 0) {
                perror("pipe");
                return 1;
            }
        }


        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }


        if (pid == 0) {

            int out_fd =
                (i < ncmds - 1)
                ? pipefd[1]
                : STDOUT_FILENO;

            run_child(
                &pl->cmds[i],
                prev_read,
                out_fd,
                pgid ? pgid : getpid()
            );
        }


        /* Parent. */

        if (pgid == 0)
            pgid = pid;

        setpgid(pid, pgid);


        if (npids < MAX_JOB_PIDS)
            pids[npids++] = pid;


        if (prev_read != STDIN_FILENO)
            close(prev_read);


        if (i < ncmds - 1) {
            close(pipefd[1]);
            prev_read = pipefd[0];
        }
    }


    /* ---------------- Background job ---------------- */

    if (pl->background) {

        int id = job_add(
            pgid,
            pids,
            npids,
            raw_line,
            1
        );

        printf("[%d] %d\n", id, pgid);

        return 0;
    }


    /* ---------------- Foreground job ---------------- */

    shell_set_foreground_pgid(pgid);

    int status = 0;
    int last_status = 0;
    int any_stopped = 0;


    for (int i = 0; i < npids; i++) {

        if (pids[i] == 0)
            continue;


        pid_t r;

        /*
         * Ctrl+C / Ctrl+Z can interrupt waitpid().
         * Retry when waitpid() returns EINTR.
         */
        do {
            r = waitpid(
                pids[i],
                &status,
                WUNTRACED
            );
        }
        while (r < 0 && errno == EINTR);


        if (r < 0) {
            perror("waitpid");
            continue;
        }


        if (WIFSTOPPED(status)) {

            any_stopped = 1;
        }
        else if (WIFEXITED(status)) {

            last_status =
                WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status)) {

            last_status =
                128 + WTERMSIG(status);
        }
    }


    /* Give terminal control back to EduShell. */

    shell_set_foreground_pgid(0);


    /*
     * Ctrl+Z stopped the foreground job.
     * Save it in the job table so jobs/bg/fg can use it.
     */

    if (any_stopped) {

        int id = job_add(
            pgid,
            pids,
            npids,
            raw_line,
            0
        );

        if (id > 0) {

            job_set_state(
                id,
                JOB_STOPPED
            );

            printf(
                "\n[%d]+  Stopped                 %s\n",
                id,
                raw_line
            );
        }
    }


    return last_status;
}
