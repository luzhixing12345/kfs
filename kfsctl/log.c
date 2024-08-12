
#include <stdio.h>
#include <unistd.h>

#include "cmd.h"
#include "ctl.h"

int log_main(int argc, const char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: kfsctl log\n");
        return -1;
    }

    ctl_cmd(CMD_LOG, NULL);

    return 0;
}