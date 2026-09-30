#ifndef EXECUTOR_H
#define EXECUTOR_H
/*
 * Week 4  - Processes & Process Control  (single command: fork/exec/wait)
 * Week 5  - fork/exec/wait in Anger      (manual PATH search, exit codes)
 * Week 7  - Pipes & Plumbing             (arbitrary-length pipelines)
 * Week 9  - Redirection & the File Abstraction (<, >, >>, 2>)
 */
#include "parser.h"

/* Manually walks $PATH looking for an executable named `cmd`
 * (Week 5). Returns a malloc'd absolute/relative path, or NULL. */
char *resolve_path(const char *cmd);

/* Runs an entire Pipeline (1..N commands). Returns the exit status
 * of the last command to report as $?. */
int execute_pipeline(Pipeline *pl, const char *raw_line);

#endif

