#include <stdio.h>
#include <unistd.h>
#include "../include/memstat.h"

void memstat_print(void) {
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) {
        printf("memstat: /proc not available on this platform\n");
        return;
    }
    char line[256];
    printf("\n----- EduShell Virtual Memory (pid %d) -----\n", getpid());
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 'V' && line[1] == 'm') {   /* VmSize, VmRSS, VmData... */
            printf("%s", line);
        }
    }
    fclose(fp);
    printf(
        "\nTip: run `valgrind --leak-check=full ./EduShell` during "
        "development - every module above allocates only through "
        "vec.c's String/Vec helpers, which makes leaks easy to spot.\n");
}
