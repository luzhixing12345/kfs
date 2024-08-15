
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include "cmd.h"
#include "ctl.h"

int restore_main(int argc, const char **argv) {
    if (ctl_init() < 0) {
        fprintf(stderr, "ctl init failed\n");
        return -1;
    }
    if (argc > 3 || argc < 2) {
        fprintf(stderr, "usage: kfsctl restore <filename> [<version>]\n");
        return -1;
    }

    const char *filename = argv[1];
    int version;
    if (argc == 3) {
        version = atoi(argv[2]);
    } else {
        version = INT_MAX;
    }

    ctl_cmd(CMD_RESTORE, filename, version);
    ctl_destroy();
    return 0;
}