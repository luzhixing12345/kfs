
#include <stdio.h>
#include <unistd.h>

#include "cmd.h"
#include "ctl.h"

int status_main(int argc, const char **argv) {
    if (ctl_init() < 0) {
        return -1;
    }

    printf("FileSystem status:\n");

    if (ctl_cmd(CMD_STATUS, NULL, -1) < 0) {
        return -1;
    }

    ctl_destroy();
    return 0;
}