#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cmd.h"
#include "ctl.h"

int add_main(int argc, const char **argv) {
    if (ctl_init() < 0) {
        fprintf(stderr, "ctl init failed\n");
        return -1;
    }

    if (argc != 2) {
        fprintf(stderr, "usage: kfsctl add <filename>\n");
        return -1;
    }

    const char *filename = argv[1];
    ctl_cmd(CMD_ADD, filename);
    printf("add %s success\n", filename);

    ctl_destroy();
    return 0;
}