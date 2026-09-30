#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "../include/vec.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/executor.h"
#include "../include/signals.h"
#include "../include/jobs.h"
#include "../include/monitor.h"

static void print_prompt(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) strcpy(cwd, "?");
    const char *base = strrchr(cwd, '/');
    printf("eduShell:%s$ ", base ? base + 1 : cwd);
    fflush(stdout);
}

int main(void) {
    printf("=========================================\n");
    printf("   Welcome to EduShell - a mini OS shell\n");
    printf("   Type 'help' for the built-in command list\n");
    printf("=========================================\n");

    jobs_init();
    shell_signals_init();   /* Week 6  */
    monitor_start();        /* Week 10 */

    char line[1024];
    while (1) {
        print_prompt();
        if (!fgets(line, sizeof(line), stdin)) {
            if (feof(stdin)) printf("\n");
            break; /* Ctrl-D */
        }

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (line[0] == '\0') continue;

        TokenList tl;
        tokenize(line, &tl);

        Pipeline pl;
        if (!parse_tokens(&tl, &pl)) {
            tokenlist_free(&tl);
            continue;
        }
        tokenlist_free(&tl);

        if (pl.ncmds > 0) {
            execute_pipeline(&pl, line);
        }
        pipeline_free(&pl);
    }

    monitor_stop();
    printf("Exiting EduShell. Goodbye!\n");
    return 0;
}
