
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>


#include "cmd.h"
#include "ctl.h"

int log_main(int argc, const char **argv) {
    if (argc != 1) {
        fprintf(stderr, "usage: kfsctl log\n");
        return -1;
    }

    if (ctl_init() < 0) {
        fprintf(stderr, "ctl init failed\n");
        return -1;
    }
    ctl_cmd(CMD_LOG, NULL);

    ctl_destroy();

    return 0;
}